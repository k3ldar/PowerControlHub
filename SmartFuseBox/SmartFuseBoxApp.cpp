
#include "SmartFuseBoxApp.h"

#include "SystemCpuMonitor.h"
#include "DateTimeManager.h"
#include "ConfigManager.h"

#if defined(CARD_CONFIG_LOADER)
SmartFuseBox* SmartFuseBoxApp::_instance = nullptr;
#endif

constexpr uint8_t DefaultDelay = 5;

SmartFuseBoxApp::SmartFuseBoxApp(SerialCommandManager* commandMgrComputer,
    SerialCommandManager* commandMgrLink,
    const uint8_t* relayPins, uint8_t relayCount)
    : _commandMgrComputer(commandMgrComputer),
      _commandMgrLink(commandMgrLink),
      _messageBus(),
      _relayController(&_messageBus, relayPins, relayCount),
      _soundController(),
      _broadcastManager(commandMgrComputer, commandMgrLink),
      _warningManager(commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs),
      _relayHandler(commandMgrComputer, commandMgrLink, &_relayController),
      _soundHandler(commandMgrComputer, commandMgrLink, &_soundController),
      _interceptDebugHandler(&_broadcastManager),
      _sensorCommandHandler(&_broadcastManager, &_warningManager),
      _warningCommandHandler(&_broadcastManager, &_warningManager),
      _ackHandler(&_broadcastManager, &_warningManager),
      _systemCommandHandler(&_broadcastManager, &_warningManager),
      _bluetoothController(&_systemCommandHandler, &_sensorCommandHandler, &_relayController, &_warningManager, commandMgrComputer),
      _wifiController(&_messageBus, commandMgrComputer, &_warningManager),
      _configController(&_soundController, &_bluetoothController, &_wifiController, &_relayController),
      _configSyncManager(commandMgrComputer, commandMgrLink, &_configController, &_warningManager),
      _configHandler(&_wifiController, &_configController),
      _configNetworkHandler(&_configController, &_wifiController),
      _relayNetworkHandler(&_relayController),
      _soundNetworkHandler(&_soundController),
      _warningNetworkHandler(&_warningManager),
      _systemNetworkHandler(&_wifiController),
      _sdCardLogger(&_sensorCommandHandler, &_warningManager),
      _sensorManager(nullptr),
      _sensorController(nullptr),
      _sensorNetworkHandler(nullptr)

#if defined(MQTT_SUPPORT)
      , _mqttController(&_messageBus, ConfigManager::getConfigPtr(), commandMgrComputer)
      , _mqttConfigHandler(&_configController, &_mqttController, commandMgrComputer)
      , _mqttRelayHandler(&_mqttController, &_messageBus, &_relayController, commandMgrComputer)
      , _mqttSensorHandler(nullptr)
      , _mqttSystemHandler(&_mqttController, &_messageBus, commandMgrComputer)
	  , _nextRunMqttMs(0)
#endif

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
      , _ledManager(&_messageBus)
#endif

#if defined(CARD_CONFIG_LOADER)
      , _sdCardConfigLoader(commandMgrComputer, commandMgrLink, &_configController, &_configSyncManager)
#endif
{
#if defined(CARD_CONFIG_LOADER)
    _instance = this;
#endif
}

void SmartFuseBoxApp::setup(BaseSensorHandler** sensorHandlers, uint8_t sensorHandlerCount,
    BaseSensor** baseSensors, uint8_t baseSensorCount)
{
    DateTimeManager::setDateTime();

    // retrieve config settings
    ConfigManager::begin();

    if (!ConfigManager::load())
    {
        _warningManager.raiseWarning(WarningType::DefaultConfigurationFuseBox);
    }

    // Create sensor management (needed before wifi/mqtt setup)
    _sensorController = new SensorController(baseSensors, baseSensorCount);
    _sensorManager = new SensorManager(sensorHandlers, sensorHandlerCount);
    _sensorNetworkHandler = new SensorNetworkHandler(_sensorController);

    _relayController.setup();

    // middleware
    _relayHandler.setup();

    // serial command handlers
    ISerialCommandHandler* linkHandlers[] = { &_relayHandler, &_soundHandler, &_configHandler, &_ackHandler, &_systemCommandHandler, &_warningCommandHandler, &_sensorCommandHandler };
    size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
    _commandMgrLink->registerHandlers(linkHandlers, linkHandlerCount);

    ISerialCommandHandler* computerHandlers[] = { &_relayHandler, &_soundHandler, &_configHandler, &_ackHandler, &_systemCommandHandler, &_warningCommandHandler, &_sensorCommandHandler };
    size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
    _commandMgrComputer->registerHandlers(computerHandlers, computerHandlerCount);

    // Link config sync manager to handlers so they can coordinate config synchronization
    _ackHandler.setConfigSyncManager(&_configSyncManager, &_configController);
    _configHandler.setConfigSyncManager(&_configSyncManager);

#if defined(MQTT_SUPPORT)
    // Link MQTT config handler and controller
    _configHandler.setMqttConfigHandler(&_mqttConfigHandler);
    _configHandler.setMqttController(&_mqttController);
#endif

#if defined(CARD_CONFIG_LOADER)
    _configHandler.setSdCardConfigLoader(&_sdCardConfigLoader);
#endif

    Config* config = ConfigManager::getConfigPtr();

    configureWifiSupport(config);
    configureBluetoothSupport(config);

#if defined(MQTT_SUPPORT)

    // Initialize MQTT (after WiFi initialization)
    _mqttSensorHandler = new MQTTSensorHandler(&_mqttController, &_messageBus, _sensorController, _commandMgrComputer);

    if (config->accessMode == AccessModeClient && _mqttController.begin())
    {
        // MQTT enabled in config, initialize handlers
        _mqttRelayHandler.begin();
        _mqttSensorHandler->begin();
        _mqttSystemHandler.begin();
    }

#endif

    _systemCommandHandler.setSdCardLogger(&_sdCardLogger);
    _systemNetworkHandler.setSdCardLogger(&_sdCardLogger);
    _soundController.configUpdated(config);
    _relayHandler.configUpdated(config);
    _sensorManager->setup();

#if defined(MQTT_SUPPORT)
    _systemCommandHandler.setMqttController(&_mqttController);
#endif

    MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
    microSdDriver.setWarningManager(&_warningManager);

#if defined(CARD_CONFIG_LOADER)
    // Register callback for SD card ready events (handles both initial init and card swaps)
    microSdDriver.setOnCardReadyCallback(onSdCardReadyCallback);
#endif

    microSdDriver.beginInitialize(SdCardCsPin, config->sdCardInitializeSpeed);

    // Initialize SD card logger
    _sdCardLogger.initialize();

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
    _ledManager.Initialize();
#endif

    // open any relays that are default open
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        if (config->defaulRelayState[i])
        {
            _relayController.setRelayState(i, true);
        }
    }

    // indicate system initialized
    _commandMgrComputer->sendCommand(SystemInitialized, "");
}

void SmartFuseBoxApp::loop()
{
    unsigned long now = millis();

    SystemCpuMonitor::startTask();
    _commandMgrComputer->readCommands();
    _commandMgrLink->readCommands();
    SystemCpuMonitor::endTask();

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
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
    _configSyncManager.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
    microSdDriver.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    _sdCardLogger.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::update();

    delay(DefaultDelay);
}

void SmartFuseBoxApp::configureWifiSupport(Config* config)
{
    // network command handlers
    INetworkCommandHandler* networkHandlers[] = { &_relayNetworkHandler, &_soundNetworkHandler, &_warningNetworkHandler,
        &_systemNetworkHandler, _sensorNetworkHandler, &_configNetworkHandler };
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
    };
    uint8_t jsonVisitorCount = sizeof(jsonVisitors) / sizeof(jsonVisitors[0]);
    _wifiController.registerJsonVisitors(jsonVisitors, jsonVisitorCount);

    _wifiController.applyConfig(config);
    _systemCommandHandler.setWifiController(&_wifiController);
}

void SmartFuseBoxApp::configureBluetoothSupport(Config* config)
{
    // bluetooth
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

