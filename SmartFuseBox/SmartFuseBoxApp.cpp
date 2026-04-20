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

#if defined(NEXTION_DISPLAY_DEVICE)
#include "NextionFactory.h"
#include <NextionControl.h>
#endif

#if defined(CARD_CONFIG_LOADER)
#include "SDCardConfigLoader.h"

SmartFuseBoxApp* SmartFuseBoxApp::_instance = nullptr;
#endif

constexpr uint8_t DefaultDelay = 5;

// Placeholder pin array used to size RelayController at construction.
// Actual pins are loaded from config in setup() via syncPinsFromConfig().
static uint8_t _disabledRelayPins[ConfigRelayCount];

SmartFuseBoxApp::SmartFuseBoxApp(SerialCommandManager* commandMgrComputer)
    : _commandMgrComputer(commandMgrComputer),
    _messageBus(),
    _relayController(&_messageBus, (memset(_disabledRelayPins, DefaultValue, sizeof(_disabledRelayPins)), _disabledRelayPins), ConfigRelayCount),
    _soundController(),
    _broadcastManager(commandMgrComputer),
    _warningManager(),
#if defined(NEXTION_DISPLAY_DEVICE)
    _nextionControl(NextionFactory::Create(&_warningManager, commandMgrComputer, &_soundController, &_relayController,
        ConfigManager::getConfigPtr() ? ConfigManager::getConfigPtr()->location.locationType : LocationType::Other, &_messageBus)),
#endif
    _relayHandler(commandMgrComputer, &_relayController),
    _soundHandler(commandMgrComputer, &_soundController),
    _interceptDebugHandler(&_broadcastManager),
    _sensorCommandHandler(&_broadcastManager,
        &_messageBus,
        &_warningManager),
    _sensorConfigHandler(commandMgrComputer),
    _warningCommandHandler(&_broadcastManager, &_warningManager),
    _ackHandler(&_broadcastManager,
        &_messageBus,
        &_warningManager),
    _systemCommandHandler(&_broadcastManager, &_warningManager),
    _bluetoothController(&_systemCommandHandler, &_sensorCommandHandler, &_relayController, &_warningManager, commandMgrComputer),
    _wifiController(&_messageBus, commandMgrComputer, &_warningManager),
    _configController(&_soundController, 
        &_bluetoothController,
        &_wifiController),
    _configHandler(&_wifiController, &_configController),
    _nextionConfigHandler(&_configController),
    _externalSensorConfigHandler(commandMgrComputer),
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
      , _gpsSerial(nullptr)
      , _remoteSensors(nullptr)
      , _remoteSensorCount(0)

#if defined(MQTT_SUPPORT)
      , _mqttController(&_messageBus, ConfigManager::getConfigPtr(), _wifiController.getRadio(), commandMgrComputer)
      , _mqttConfigHandler(&_configController, &_mqttController, commandMgrComputer)
      , _mqttRelayHandler(&_mqttController, &_messageBus, &_relayController, commandMgrComputer)
      , _mqttSensorHandler(nullptr)
      , _mqttSystemHandler(&_mqttController, &_messageBus, commandMgrComputer)
	  , _nextRunMqttMs(0)
#endif

      , _schedulerCommandHandler(&_relayController)
      , _serialHandlerCount(0)
      , _scheduleController(&_relayController, &_messageBus)
      , _pendingRebootTime(0)

#if defined(LED_MANAGER)
      , _ledManager(&_messageBus)
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
      , _otaManager(&_wifiController, &_broadcastManager)
#endif

#if defined(CARD_CONFIG_LOADER)
      , _sdCardConfigLoader(commandMgrComputer, &_configController, &_relayController)
#endif
{
#if defined(CARD_CONFIG_LOADER)
    _instance = this;
#endif
}

void SmartFuseBoxApp::setup(RemoteSensor** remoteSensors, uint8_t remoteSensorCount)
{
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

	Config* config = ConfigManager::getConfigPtr();

	DateTimeManager::begin(config->rtc);

	// Build remote sensors from RemoteSensorsConfig (populated by ConfigManager::load()).
	// Sensors are created dynamically here; ownership stays with SmartFuseBoxApp (_remoteSensors).
	// Any sensors passed in by the caller are ignored when config has entries.
	if (config->remoteSensors.count > 0)
	{
		_remoteSensorCount = config->remoteSensors.count;
		_remoteSensors = new RemoteSensor*[_remoteSensorCount];

#if defined(MQTT_SUPPORT)
		// Allocate one MqttSensorChannel per sensor; stored in a flat array.
		MqttSensorChannel* channels = new MqttSensorChannel[_remoteSensorCount];
#endif

		for (uint8_t i = 0; i < _remoteSensorCount; i++)
		{
			const RemoteSensorConfig& entry = config->remoteSensors.sensors[i];

#if defined(MQTT_SUPPORT)
			channels[i].name        = entry.mqttName;
			channels[i].slug        = entry.mqttSlug;
			channels[i].typeSlug    = entry.mqttTypeSlug;
			channels[i].deviceClass = entry.mqttDeviceClass[0] != '\0' ? entry.mqttDeviceClass : nullptr;
			channels[i].unit        = entry.mqttUnit[0]        != '\0' ? entry.mqttUnit        : nullptr;
			channels[i].isBinary    = entry.mqttIsBinary;

			_remoteSensors[i] = new RemoteSensor(
				entry.sensorId,
				entry.name,
				entry.name,     // commandId: external devices push updates using the sensor name
				entry.name,     // mqttTopic
				&channels[i],
				1);
#else
			_remoteSensors[i] = new RemoteSensor(
				entry.sensorId,
				entry.name,
				entry.name,     // commandId
				1);
#endif
		}

		remoteSensors     = _remoteSensors;
		remoteSensorCount = _remoteSensorCount;
		_sensorCommandHandler.setup(_remoteSensors, _remoteSensorCount);
	}


    // Build local sensors from SensorsConfig (populated by ConfigManager::load() above).
    // SensorFactory allocates each enabled entry once; ownership stays with SmartFuseBoxApp.
    {
        SensorFactoryContext ctx;
        ctx.messageBus = &_messageBus;
        ctx.broadcastManager = &_broadcastManager;
        ctx.sensorCommandHandler = &_sensorCommandHandler;
        ctx.warningManager = &_warningManager;
        ctx.relayController = &_relayController;
        ctx.wifiController = &_wifiController;
        ctx.bluetoothRadio = &_bluetoothController;
        ctx.gpsSerial = _gpsSerial;
#if defined(SD_CARD_SUPPORT)
        ctx.sdCardLogger = &_sdCardLogger;
#endif

        _factorySensors = SensorFactory::create(config->sensors, ctx, _factorySensorCount);
    }

    // Merge factory-built local sensors
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
    _serialHandlerCount = 0;
    _serialHandlers[_serialHandlerCount++] = &_relayHandler;
    _serialHandlers[_serialHandlerCount++] = &_soundHandler;
    _serialHandlers[_serialHandlerCount++] = &_configHandler;
    _serialHandlers[_serialHandlerCount++] = &_nextionConfigHandler;
    _serialHandlers[_serialHandlerCount++] = &_externalSensorConfigHandler;
    _serialHandlers[_serialHandlerCount++] = &_ackHandler;
    _serialHandlers[_serialHandlerCount++] = &_systemCommandHandler;
    _serialHandlers[_serialHandlerCount++] = &_warningCommandHandler;
    _serialHandlers[_serialHandlerCount++] = &_sensorCommandHandler;
    _serialHandlers[_serialHandlerCount++] = &_sensorConfigHandler;
    _serialHandlers[_serialHandlerCount++] = &_schedulerCommandHandler;

    _commandMgrComputer->registerHandlers(_serialHandlers, _serialHandlerCount);

    // Give the WiFi command bridge
    // POST /api/command/{cmd} delegates to the exact same implementations.
    _wifiCommandBridge.setHandlers(_serialHandlers, _serialHandlerCount);

    // Link config sync manager to handlers so they can coordinate config synchronization
    _ackHandler.setConfigController(&_configController);
    _configHandler.setMessageBus(&_messageBus);

    // Subscribe to deferred reboot requests
    _messageBus.subscribe<RebootRequested>([this]()
    {
        _pendingRebootTime = SystemFunctions::millis64() + 500;
    });

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

    if (config->sdCard.csPin != PinDisabled)
    {
        microSdDriver.beginInitialize(config->spiPins.misoPin, config->spiPins.mosiPin, config->spiPins.sckPin, config->sdCard.csPin, config->sdCard.initializeSpeed);
    }

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
    uint64_t now = SystemFunctions::millis64();

    SystemCpuMonitor::startTask();
    _commandMgrComputer->readCommands();
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

    if (_pendingRebootTime != 0 && now >= _pendingRebootTime)
    {
        SystemFunctions::reboot();
    }

    delay(DefaultDelay);
}

void SmartFuseBoxApp::configureWifiSupport(Config* config)
{
    // network command handlers
    INetworkCommandHandler* networkHandlers[] = { &_relayNetworkHandler, &_soundNetworkHandler, &_warningNetworkHandler,
        &_systemNetworkHandler, _sensorNetworkHandler, &_configNetworkHandler, &_schedulerNetworkHandler,
        &_externalSensorNetworkHandler,
        &_wifiCommandBridge
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
        &_externalSensorNetworkHandler,
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

