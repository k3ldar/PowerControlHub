#include <ArduinoBLE.h>
#include <Arduino.h>
#include <stdint.h>
#include <SerialCommandManager.h>
#include <SPI.h>
#include <SdFat.h>

#include "SystemCpuMonitor.h"
#include "DateTimeManager.h"

#include "SmartFuseBoxConstants.h"
#include "Config.h"
#include "ConfigManager.h"
#include "ConfigCommandHandler.h"
#include "WarningManager.h"

#include "SystemCommandHandler.h"
#include "AckCommandHandler.h"
#include "SoundCommandHandler.h"
#include "RelayCommandHandler.h"
#include "BaseCommandHandler.h"
#include "SensorCommandHandler.h"
#include "WarningCommandHandler.h"
#include "InterceptDebugCommandHandler.h"

#include "WaterSensorHandler.h"
#include "Dht11SensorHandler.h"
#include "LightSensorHandler.h"
#include <SensorManager.h>

#include "BluetoothManager.h"
#include "BluetoothSystemService.h"
#include "BluetoothSensorService.h"
#include "BluetoothController.h"

#include "WifiController.h"
#include "ConfigNetworkHandler.h"
#include "RelayNetworkHandler.h"
#include "SoundNetworkHandler.h"
#include "SystemNetworkHandler.h"
#include "SensorNetworkHandler.h"
#include "WarningNetworkHandler.h"

#include "ConfigController.h"
#include "ConfigSyncManager.h"
#include "SensorController.h"
#include "SoundController.h"

#include "MicroSdDriver.h"

#if defined(MQTT_SUPPORT)
#include "MQTTHandler.h"
#include "MQTTController.h"
#include "MQTTConfigCommandHandler.h"
#include "MQTTRelayHandler.h"
#include "MQTTSensorHandler.h"
#endif

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
#include "LedMatrixManager.h"
#endif

#include "MessageBus.h"
#include "SensorDataRecord.h"
#include "MicroSdDriver.h"
#include "SdCardLogger.h"

#if defined(CARD_CONFIG_LOADER)
#include "SDCardConfigLoader.h"
#endif

#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

// forward declares
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);
void configureWifiSupport(Config* config);
void configureBluetoothSupport(Config* config);

#if defined(CARD_CONFIG_LOADER)
void onSdCardReady(MicroSdDriver* driver, bool isNewCard);
#endif

// message bus
MessageBus messageBus;

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
// led
LedMatrixManager ledManager(&messageBus);
#endif

// controllers
RelayController relayController(&messageBus, Relays, TotalRelays);
SoundController soundController;

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', ';', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', ';', '=', 500, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

// Warning manager with heartbeat monitoring
WarningManager warningManager(&commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs);

// link command handlers
RelayCommandHandler relayHandler(&commandMgrComputer, &commandMgrLink, &relayController);
SoundCommandHandler soundHandler(&commandMgrComputer, &commandMgrLink, &soundController);
InterceptDebugHandler interceptDebugHandler(&broadcastManager);
SensorCommandHandler sensorCommandHandler(&broadcastManager, &warningManager);
WarningCommandHandler warningCommandHandler(&broadcastManager, &warningManager);

// shared command handlers
AckCommandHandler ackHandler(&broadcastManager, &warningManager);
SystemCommandHandler systemCommandHandler(&broadcastManager, &warningManager);

// Sensors
WaterSensorHandler waterSensorHandler(&messageBus, &broadcastManager, &sensorCommandHandler, WaterSensorPin, WaterSensorActivePin);
Dht11SensorHandler dht11SensorHandler(&messageBus, &broadcastManager, &sensorCommandHandler, &warningManager, Dht11SensorPin);
LightSensorHandler lightSensorHandler(&messageBus, &broadcastManager, &sensorCommandHandler, &warningManager, LightSensorPin, LightSensorAnalogPin);

BaseSensorHandler* sensorHandlers[] = {
	&waterSensorHandler, &dht11SensorHandler, &lightSensorHandler
};
uint8_t sensorHandlerCount = sizeof(sensorHandlers) / sizeof(sensorHandlers[0]);
SensorManager sensorManager(sensorHandlers, sensorHandlerCount);

// configure bluetooth support
BluetoothController bluetoothController(&systemCommandHandler, &sensorCommandHandler, &relayController, &warningManager, &commandMgrComputer);

WifiController wifiController(&messageBus, &commandMgrComputer, &warningManager);
ConfigController configController(&soundController, &bluetoothController, &wifiController, &relayController);

ConfigSyncManager configSyncManager(&commandMgrComputer, &commandMgrLink, &configController, &warningManager);

// computer command handlers
ConfigCommandHandler configHandler(&wifiController, &configController);

// middleware
BaseSensor* baseSensors[] = {
	&waterSensorHandler, &dht11SensorHandler, &lightSensorHandler
};
uint8_t baseSensorCount = sizeof(baseSensors) / sizeof(baseSensors[0]);
SensorController sensorController(baseSensors, baseSensorCount);

// MQTT instances
#if defined(MQTT_SUPPORT)
MQTTController mqttController(&messageBus, ConfigManager::getConfigPtr(), &commandMgrComputer);
MQTTConfigCommandHandler mqttConfigHandler(&configController, &mqttController, &commandMgrComputer);
MQTTRelayHandler mqttRelayHandler(&mqttController, &messageBus, &relayController, &commandMgrComputer);
MQTTSensorHandler mqttSensorHandler(&mqttController, &messageBus, &sensorController, &commandMgrComputer);
#endif

// configure wifi support
ConfigNetworkHandler configNetworkHandler(&configController, &wifiController);
RelayNetworkHandler relayNetworkHandler(&relayController);
SoundNetworkHandler soundNetworkHandler(&soundController);
WarningNetworkHandler warningNetworkHandler(&warningManager);
SystemNetworkHandler systemNetworkHandler(&wifiController);
SensorNetworkHandler sensorNetworkHandler(&sensorController);

// SD card logger
SdCardLogger sdCardLogger(&sensorCommandHandler, &warningManager);

#if defined(CARD_CONFIG_LOADER)
SdCardConfigLoader sdCardConfigLoader(&commandMgrComputer, &commandMgrLink, &configController, &configSyncManager);
#endif

void setup()
{
	// Serial initialization is performed first to ensure that any logging or error messages
	// from DateTimeManager or ConfigManager during initialization are properly output.
	SystemFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);
	SystemFunctions::initializeSerial(LINK_SERIAL, 19200, true);

	DateTimeManager::setDateTime();

	// retrieve config settings
	ConfigManager::begin();

	if (!ConfigManager::load())
	{
		warningManager.raiseWarning(WarningType::DefaultConfigurationFuseBox);
	}

	relayController.setup();

	// middleware
	relayHandler.setup();

	// serial command handlers
	ISerialCommandHandler* linkHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemCommandHandler, &warningCommandHandler, &sensorCommandHandler } ;
	size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
	commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

	ISerialCommandHandler* computerHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemCommandHandler, &warningCommandHandler, &sensorCommandHandler };
	size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
	commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

	// Link config sync manager to handlers so they can coordinate config synchronization
	ackHandler.setConfigSyncManager(&configSyncManager, &configController);
	configHandler.setConfigSyncManager(&configSyncManager);

#if defined(MQTT_SUPPORT)
	// Link MQTT config handler and controller
	configHandler.setMqttConfigHandler(&mqttConfigHandler);
	configHandler.setMqttController(&mqttController);
#endif

#if defined(CARD_CONFIG_LOADER)
	configHandler.setSdCardConfigLoader(&sdCardConfigLoader);
#endif

	Config* config = ConfigManager::getConfigPtr();

	configureWifiSupport(config);
	configureBluetoothSupport(config);

#if defined(MQTT_SUPPORT)

	// Initialize MQTT (after WiFi initialization)
	if (config->accessMode == AccessModeClient && mqttController.begin())
	{
		// MQTT enabled in config, initialize handlers
		mqttRelayHandler.begin();
		mqttSensorHandler.begin();
	}

#endif

	systemCommandHandler.setSdCardLogger(&sdCardLogger);
	systemNetworkHandler.setSdCardLogger(&sdCardLogger);
	soundController.configUpdated(config);
	relayHandler.configUpdated(config);
	sensorManager.setup();

#if defined(MQTT_SUPPORT)
	systemCommandHandler.setMqttController(&mqttController);
#endif

	MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
	microSdDriver.setWarningManager(&warningManager);

#if defined(CARD_CONFIG_LOADER)
	// Register callback for SD card ready events (handles both initial init and card swaps)
	microSdDriver.setOnCardReadyCallback(onSdCardReady);
#endif

	microSdDriver.beginInitialize(SdCardCsPin, config->sdCardInitializeSpeed);

	// Initialize SD card logger
	sdCardLogger.initialize();

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
	ledManager.Initialize();
#endif

	// open any relays that are default open
	for (uint8_t i = 0; i < ConfigRelayCount; i++)
	{
		if (config->defaulRelayState[i])
		{
			relayController.setRelayState(i, true);
		}
	}

	// indicate system initialized
	commandMgrComputer.sendCommand(SystemInitialized, "");
}

void loop() 
{
	unsigned long now = millis();

	SystemCpuMonitor::startTask();
	commandMgrComputer.readCommands();
	commandMgrLink.readCommands();
	SystemCpuMonitor::endTask();

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
	SystemCpuMonitor::startTask();
	ledManager.ProcessLedMatrix(now);
	SystemCpuMonitor::endTask();
#endif

	SystemCpuMonitor::startTask();
	soundController.update();
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	sensorManager.update(now);
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	bluetoothController.loop();
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	wifiController.update(now);
	SystemCpuMonitor::endTask();

	// MQTT update (non-blocking, processes 1 packet per call)
#if defined(MQTT_SUPPORT)
	SystemCpuMonitor::startTask();
	mqttController.update();
	mqttRelayHandler.update(); 
	mqttSensorHandler.update();
	SystemCpuMonitor::endTask();
#endif

	SystemCpuMonitor::startTask();
	configSyncManager.update(now);
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	MicroSdDriver& microSdDriver = MicroSdDriver::getInstance();
	microSdDriver.update(now);
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	sdCardLogger.update(now);
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::update();
	delay(DefaultDelay);
}

void onComputerCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATCMD"));
	SystemFunctions::resetSerial(COMPUTER_SERIAL);
}

void onLinkCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATLNK"));
	SystemFunctions::resetSerial(LINK_SERIAL);
}

void configureWifiSupport(Config* config)
{
	// network command handlers
	INetworkCommandHandler* networkHandlers[] = { &relayNetworkHandler, &soundNetworkHandler, &warningNetworkHandler,
		&systemNetworkHandler, &sensorNetworkHandler, &configNetworkHandler };
	size_t networkHandlerCount = sizeof(networkHandlers) / sizeof(networkHandlers[0]);
	wifiController.registerHandlers(networkHandlers, networkHandlerCount);


	// json status visitors for wifi
	NetworkJsonVisitor* jsonVisitors[] = {
		&systemNetworkHandler,
		&configNetworkHandler,
		&relayNetworkHandler,
		&soundNetworkHandler,
		&warningNetworkHandler,
		&sensorNetworkHandler,
	};
	uint8_t jsonVisitorCount = sizeof(jsonVisitors) / sizeof(jsonVisitors[0]);
	wifiController.registerJsonVisitors(jsonVisitors, jsonVisitorCount);

	wifiController.applyConfig(config);
	systemCommandHandler.setWifiController(&wifiController);
}

void configureBluetoothSupport(Config* config)
{
	// bluetooth
	bluetoothController.applyConfig(config);
}

#if defined(CARD_CONFIG_LOADER)
void onSdCardReady(bool isNewCard)
{
	// Forward the callback to the config loader
	sdCardConfigLoader.onSdCardReady(isNewCard);
}
#endif
