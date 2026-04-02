/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SmartFuseBoxApp.h"

#include "SystemCpuMonitor.h"
#include "DateTimeManager.h"
#include "ConfigManager.h"
#include "SensorFactory.h"

#if defined(CARD_CONFIG_LOADER)
#include "SDCardConfigLoader.h"

SmartFuseBoxApp* SmartFuseBoxApp::_instance = nullptr;
#endif

constexpr uint8_t DefaultDelay = 5;

// Placeholder pin array used to size RelayController at construction.
// Actual pins are loaded from config in setup() via syncPinsFromConfig().
static uint8_t _disabledRelayPins[ConfigRelayCount];

SmartFuseBoxApp::SmartFuseBoxApp(SerialCommandManager* commandMgrComputer,
    SerialCommandManager* commandMgrLink)
    : _commandMgrComputer(commandMgrComputer),
    _commandMgrLink(commandMgrLink),
    _messageBus(),
    _relayController(&_messageBus, (memset(_disabledRelayPins, DefaultValue, sizeof(_disabledRelayPins)), _disabledRelayPins), ConfigRelayCount),
    _soundController(),
    _broadcastManager(commandMgrComputer, commandMgrLink),
    _warningManager(commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs),
    _relayHandler(commandMgrComputer, commandMgrLink, &_relayController),
    _soundHandler(commandMgrComputer, commandMgrLink, &_soundController),
    _interceptDebugHandler(&_broadcastManager),
    _sensorCommandHandler(&_broadcastManager, &_warningManager),
    _sensorConfigHandler(commandMgrComputer, commandMgrLink),
    _warningCommandHandler(&_broadcastManager, &_warningManager),
    _ackHandler(&_broadcastManager, &_warningManager),
    _systemCommandHandler(&_broadcastManager, &_warningManager),
    _bluetoothController(&_systemCommandHandler, &_sensorCommandHandler, &_relayController, &_warningManager, commandMgrComputer),
    _wifiController(&_messageBus, commandMgrComputer, &_warningManager),
    _configController(&_soundController, 
        &_bluetoothController,
        &_wifiController),
    _configHandler(&_wifiController, &_configController),
    _configNetworkHandler(&_configController, &_wifiController, &_relayController),
    _relayNetworkHandler(&_relayController),
    _soundNetworkHandler(&_soundController),
    _warningNetworkHandler(&_warningManager),
    _systemNetworkHandler(&_wifiController),
    _sensorNetworkHandler(nullptr)

#if defined(SD_CARD_SUPPORT)
      , _sdCardLogger(&_sensorCommandHandler, &_warningManager)
#endif

      , _sensorManager(nullptr)
      , _sensorController(nullptr)
      , _factorySensors(nullptr)
      , _factorySensorCount(0)

#if defined(MQTT_SUPPORT)
      , _mqttController(&_messageBus, ConfigManager::getConfigPtr(), _wifiController.getRadio(), commandMgrComputer)
      , _mqttConfigHandler(&_configController, &_mqttController, commandMgrComputer)
      , _mqttRelayHandler(&_mqttController, &_messageBus, &_relayController, commandMgrComputer)
      , _mqttSensorHandler(nullptr)
      , _mqttSystemHandler(&_mqttController, &_messageBus, commandMgrComputer)
	  , _nextRunMqttMs(0)
#endif

      , _schedulerCommandHandler(&_relayController)
      , _scheduleController(&_relayController, &_messageBus)

#if defined(LED_MANAGER)
      , _ledManager(&_messageBus)
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
      , _otaManager(&_wifiController, &_broadcastManager)
#endif

#if defined(CARD_CONFIG_LOADER)
      , _sdCardConfigLoader(commandMgrComputer, commandMgrLink, &_configController, &_relayController)
#endif
{
#if defined(CARD_CONFIG_LOADER)
    _instance = this;
#endif
}

void SmartFuseBoxApp::setup(RemoteSensor** remoteSensors, uint8_t remoteSensorCount)
{
    DateTimeManager::setDateTime();

    // retrieve config settings
    ConfigManager::begin();

    if (!ConfigManager::load())
    {
        _warningManager.raiseWarning(WarningType::DefaultConfigurationFuseBox);
    }

    // Sync relay pin assignments from config (overrides the bootstrap pins
    // passed at construction; disabled relays will have pin == 0xFF).
    _relayController.syncPinsFromConfig();
    _relayController.setSoundController(&_soundController);

	if (remoteSensorCount > 0 && remoteSensors != nullptr)
    {
        _sensorCommandHandler.setup(remoteSensors, remoteSensorCount);
    }

    // Build local sensors from SensorsConfig (populated by ConfigManager::load() above).
    // SensorFactory allocates each enabled entry once; ownership stays with SmartFuseBoxApp.
    Config* config = ConfigManager::getConfigPtr();

    {
        SensorFactoryContext ctx;
        ctx.messageBus           = &_messageBus;
        ctx.broadcastManager     = &_broadcastManager;
        ctx.sensorCommandHandler = &_sensorCommandHandler;
        ctx.warningManager       = &_warningManager;
        ctx.relayController      = &_relayController;
        ctx.wifiController       = &_wifiController;
        ctx.bluetoothRadio       = &_bluetoothController;
#if defined(SD_CARD_SUPPORT)
        ctx.sdCardLogger         = &_sdCardLogger;
#endif

        _factorySensors = SensorFactory::create(config->sensors, ctx, _factorySensorCount);
    }

    // Merge factory-built local sensors and caller-supplied remote sensors into
    // flat arrays for SensorController (query/status) and SensorManager (poll loop).
    uint8_t totalCount = _factorySensorCount + remoteSensorCount;
    BaseSensor**        allSensors  = (totalCount > 0) ? new BaseSensor*[totalCount]        : nullptr;
    BaseSensorHandler** allHandlers = (totalCount > 0) ? new BaseSensorHandler*[totalCount] : nullptr;

    for (uint8_t i = 0; i < _factorySensorCount; i++)
    {
        _factorySensors[i]->setUniqueId(i);
        allSensors[i]  = _factorySensors[i];
        allHandlers[i] = static_cast<BaseSensorHandler*>(_factorySensors[i]);
    }

    for (uint8_t i = 0; i < remoteSensorCount; i++)
    {
        uint8_t idx = _factorySensorCount + i;
        remoteSensors[i]->setUniqueId(idx);
        allSensors[idx]  = remoteSensors[i];
        allHandlers[idx] = static_cast<BaseSensorHandler*>(remoteSensors[i]);
    }

    _sensorController = new SensorController(allSensors, totalCount);
    _sensorManager    = new SensorManager(allHandlers, totalCount);

    _sensorNetworkHandler = new SensorNetworkHandler(_sensorController);

    _relayController.setup();

    // middleware
    _relayHandler.setup();

    // serial command handlers
    ISerialCommandHandler* linkHandlers[] = { &_relayHandler, &_soundHandler, &_configHandler, &_ackHandler,
        &_systemCommandHandler, &_warningCommandHandler, &_sensorCommandHandler, &_sensorConfigHandler, &_schedulerCommandHandler
        };
    size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
    _commandMgrLink->registerHandlers(linkHandlers, linkHandlerCount);

    ISerialCommandHandler* computerHandlers[] = { &_relayHandler, &_soundHandler, &_configHandler, &_ackHandler,
        &_systemCommandHandler, &_warningCommandHandler, &_sensorCommandHandler, &_sensorConfigHandler, &_schedulerCommandHandler 
        };
    size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
    _commandMgrComputer->registerHandlers(computerHandlers, computerHandlerCount);

    // Link config sync manager to handlers so they can coordinate config synchronization
    _ackHandler.setConfigController(&_configController);

#if defined(MQTT_SUPPORT)
    // Link MQTT config handler and controller
    _configHandler.setMqttConfigHandler(&_mqttConfigHandler);
    _configHandler.setMqttController(&_mqttController);
#endif

#if defined(CARD_CONFIG_LOADER)
    _configHandler.setSdCardConfigLoader(&_sdCardConfigLoader);
#endif

    configureWifiSupport(config);
    configureBluetoothSupport(config);

#if defined(MQTT_SUPPORT)

    // Initialize MQTT (after WiFi initialization)
    _mqttSensorHandler = new MQTTSensorHandler(&_mqttController, &_messageBus, _sensorController, _commandMgrComputer);

    if (config->network.accessMode == WifiMode::Client && _mqttController.begin())
    {
        // MQTT enabled in config, initialize handlers
        _mqttRelayHandler.begin();
        _mqttSensorHandler->begin();
        _mqttSystemHandler.begin();
    }

#endif

#if defined(SD_CARD_SUPPORT)
    _systemCommandHandler.setSdCardLogger(&_sdCardLogger);
    _systemNetworkHandler.setSdCardLogger(&_sdCardLogger);
#endif

    _soundController.configUpdated(config);
    _relayHandler.configUpdated(config);
    _sensorManager->setup();

#if defined(MQTT_SUPPORT)
    _systemCommandHandler.setMqttController(&_mqttController);
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
    _systemCommandHandler.setOtaManager(&_otaManager);
    _otaManager.begin();
#endif

#if defined(SD_CARD_SUPPORT)
    MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
    microSdDriver.setWarningManager(&_warningManager);

#if defined(CARD_CONFIG_LOADER)
    // Register callback for SD card ready events (handles both initial init and card swaps)
    microSdDriver.setOnCardReadyCallback(onSdCardReadyCallback);
#endif

    microSdDriver.beginInitialize(SdCardCsPin, config->sdCard.initializeSpeed);

    // Initialize SD card logger
    _sdCardLogger.initialize();
#endif

#if defined(LED_MANAGER)
    _ledManager.Initialize();
#endif

    // open any relays that are default open
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        if (!_relayController.isDisabled(i) && config->relay.relays[i].defaultState)
        {
            _relayController.setRelayState(i, true);
        }
    }

    // indicate system initialized
    _commandMgrComputer->sendCommand(SystemInitialized, "");
    _scheduleController.begin();
}

void SmartFuseBoxApp::loop()
{
    unsigned long now = millis();

    SystemCpuMonitor::startTask();
    _commandMgrComputer->readCommands();
    _commandMgrLink->readCommands();
    SystemCpuMonitor::endTask();

#if defined(LED_MANAGER)
    SystemCpuMonitor::startTask();
    _ledManager.ProcessLedMatrix(now);
    SystemCpuMonitor::endTask();
#endif

    SystemCpuMonitor::startTask();
    _soundController.update();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    if (_sensorManager)
    {
        _sensorManager->update(now);
    }
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _bluetoothController.loop();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _wifiController.update(now);
    SystemCpuMonitor::endTask();

    // MQTT update (non-blocking, processes 1 packet per call)
#if defined(MQTT_SUPPORT)
    SystemCpuMonitor::startTask();
    _mqttController.update();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _mqttRelayHandler.update();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    if (_mqttSensorHandler)
    {
        _mqttSensorHandler->update();
    }
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _mqttSystemHandler.update();
    SystemCpuMonitor::endTask();
#endif

    SystemCpuMonitor::startTask();
    _scheduleController.update(now);
    SystemCpuMonitor::endTask();

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
    SystemCpuMonitor::startTask();
    _otaManager.update(now);
    SystemCpuMonitor::endTask();
#endif

#if defined(SD_CARD_SUPPORT)
    SystemCpuMonitor::startTask();
    MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
    microSdDriver.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _sdCardLogger.update(now);
    SystemCpuMonitor::endTask();
#endif

    SystemCpuMonitor::update();

    delay(DefaultDelay);
}

void SmartFuseBoxApp::configureWifiSupport(Config* config)
{
    // network command handlers
    INetworkCommandHandler* networkHandlers[] = { &_relayNetworkHandler, &_soundNetworkHandler, &_warningNetworkHandler,
        &_systemNetworkHandler, _sensorNetworkHandler, &_configNetworkHandler, &_schedulerNetworkHandler
    };
    size_t networkHandlerCount = sizeof(networkHandlers) / sizeof(networkHandlers[0]);
    _wifiController.registerHandlers(networkHandlers, networkHandlerCount);

    // json status visitors for wifi
    NetworkJsonVisitor* jsonVisitors[] = {
        &_systemNetworkHandler,
        &_configNetworkHandler,
        &_relayNetworkHandler,
        &_soundNetworkHandler,
        &_warningNetworkHandler,
        _sensorNetworkHandler,
        &_schedulerNetworkHandler,
    };
    uint8_t jsonVisitorCount = sizeof(jsonVisitors) / sizeof(jsonVisitors[0]);
    _wifiController.registerJsonVisitors(jsonVisitors, jsonVisitorCount);

    _wifiController.applyConfig(config);

#if defined(WIFI_SUPPORT)
    _systemCommandHandler.setWifiController(&_wifiController);
#endif
}

void SmartFuseBoxApp::configureBluetoothSupport(Config* config)
{
    _bluetoothController.applyConfig(config);
}

#if defined(CARD_CONFIG_LOADER)
void SmartFuseBoxApp::onSdCardReadyCallback(bool isNewCard)
{
    if (_instance)
    {
        _instance->_sdCardConfigLoader.onSdCardReady(isNewCard);
    }
}
#endif

