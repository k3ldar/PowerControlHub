#include <ArduinoBLE.h>
#include <Arduino.h>
#include <stdint.h>
#include <SerialCommandManager.h>

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
#include "SensorManager.h"

#include "BluetoothManager.h"
#include "BluetoothSystemService.h"
#include "BluetoothSensorService.h"
#include "BluetoothController.h"

#include "WifiController.h"
#include "RelayNetworkHandler.h"
#include "SoundNetworkHandler.h"
#include "WarningNetworkHandler.h"
#include "SystemNetworkHandler.h"
#include "SensorNetworkHandler.h"

#include "RelayController.h"
#include "SensorController.h"
#include "SoundController.h"


#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

// forward declares
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);
void configureWifiSupport(Config* config);
void configureBluetoothSupport(Config* config);

// controllers
RelayController relayController(Relays, TotalRelays);
SoundController soundController;

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', '=', 500, 64);

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
WaterSensorHandler waterSensorHandler(&broadcastManager, &sensorCommandHandler, WaterSensorPin, WaterSensorActivePin);
Dht11SensorHandler dht11SensorHandler(&broadcastManager, &sensorCommandHandler, &warningManager, Dht11SensorPin);

BaseSensorHandler* sensorHandlers[] = {
	&waterSensorHandler, &dht11SensorHandler
};
uint8_t sensorHandlerCount = sizeof(sensorHandlers) / sizeof(sensorHandlers[0]);
SensorManager sensorManager(sensorHandlers, sensorHandlerCount);

// configure bluetooth support
BluetoothController bluetoothController(&systemCommandHandler, &sensorCommandHandler, &relayController, &warningManager, &commandMgrComputer);

WifiController wifiController(&commandMgrComputer, &warningManager);

// computer command handlers
ConfigCommandHandler configHandler(&soundController, &bluetoothController, &wifiController, &relayHandler);

// middleware
BaseSensor* baseSensors[] = {
	&waterSensorHandler, &dht11SensorHandler
};
uint8_t baseSensorCount = sizeof(baseSensors) / sizeof(baseSensors[0]);
SensorController sensorController(baseSensors, sensorHandlerCount);


// configure wifi support
RelayNetworkHandler relayNetworkHandler(&relayController);
SoundNetworkHandler soundNetworkHandler(&soundController);
WarningNetworkHandler warningNetworkHandler(&warningManager);
SystemNetworkHandler systemNetworkHandler(&wifiController);
SensorNetworkHandler sensorNetworkHandler(&sensorController);

void setup()
{
	// Serial initialization is performed first to ensure that any logging or error messages
	// from DateTimeManager or ConfigManager during initialization are properly output.
	SharedFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);
	SharedFunctions::initializeSerial(LINK_SERIAL, 9600, true);

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

	Config* config = ConfigManager::getConfigPtr();

	configureWifiSupport(config);
	configureBluetoothSupport(config);
	soundController.configUpdated(config);
	relayHandler.configUpdated(config);
	sensorManager.setup();

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
	wifiController.update();
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::update();
	delay(DefaultDelay);
}

void onComputerCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATCMD"));
}

void onLinkCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATLNK"));
}

void configureWifiSupport(Config* config)
{
	// network command handlers
	INetworkCommandHandler* networkHandlers[] = { &relayNetworkHandler, &soundNetworkHandler, &warningNetworkHandler,
		&systemNetworkHandler, &sensorNetworkHandler };
	size_t networkHandlerCount = sizeof(networkHandlers) / sizeof(networkHandlers[0]);
	wifiController.registerHandlers(networkHandlers, networkHandlerCount);


	// json status visitors for wifi
	NetworkJsonVisitor* jsonVisitors[] = {
		&relayNetworkHandler,
		&soundNetworkHandler,
		&warningNetworkHandler,
		&systemNetworkHandler,
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
