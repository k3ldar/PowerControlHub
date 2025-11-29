#include <ArduinoBLE.h>
#include <Arduino.h>
#include <stdint.h>
#include <SerialCommandManager.h>

#include "SystemCpuMonitor.h"

#include "SmartFuseBoxConstants.h"
#include "Config.h"
#include "ConfigManager.h"
#include "ConfigCommandHandler.h"
#include "SoundManager.h"
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


#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

// forward declares
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', '=', 500, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

// Warning manager with heartbeat monitoring
WarningManager warningManager(&commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs);

SoundManager soundManager;

// link command handlers
RelayCommandHandler relayHandler(&commandMgrComputer, &commandMgrLink, Relays, TotalRelays);
SoundCommandHandler soundHandler(&commandMgrComputer, &commandMgrLink, &soundManager);
InterceptDebugHandler interceptDebugHandler(&broadcastManager);
SensorCommandHandler sensorCommandHandler(&broadcastManager, &warningManager);
WarningCommandHandler warningCommandHandler(&broadcastManager, &warningManager);

// shared command handlers
AckCommandHandler ackHandler(&broadcastManager, &warningManager);
SystemCommandHandler systemCommandHandler(&broadcastManager);

// Sensors
WaterSensorHandler waterSensorHandler(&broadcastManager, &sensorCommandHandler, WaterSensorPin, WaterSensorActivePin);
Dht11SensorHandler dht11SensorHandler(&broadcastManager, &sensorCommandHandler, &warningManager, Dht11SensorPin);

BaseSensorHandler* sensorHandlers[] = {
	&waterSensorHandler, &dht11SensorHandler
};
SensorManager sensorManager(sensorHandlers, sizeof(sensorHandlers) / sizeof(sensorHandlers[0]));

// configure bluetooth support
BluetoothController bluetoothController(&systemCommandHandler, &sensorCommandHandler, &relayHandler, &warningManager, &commandMgrComputer);

// computer command handlers
ConfigCommandHandler configHandler(&soundManager, &bluetoothController, &relayHandler);

void setup()
{
	// retrieve config settings
	ConfigManager::begin();

	if (!ConfigManager::load())
	{
		warningManager.raiseWarning(WarningType::DefaultConfigurationFuseBox);
	}

	ISerialCommandHandler* linkHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemCommandHandler, &warningCommandHandler, &sensorCommandHandler } ;
	size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
	commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

	ISerialCommandHandler* computerHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemCommandHandler, &warningCommandHandler, &sensorCommandHandler };
	size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
	commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

	SharedFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);
	SharedFunctions::initializeSerial(LINK_SERIAL, 9600, true);

	Config* config = ConfigManager::getConfigPtr();
	bluetoothController.applyConfig(config);
	relayHandler.setup();

	soundManager.configUpdated(config);
	relayHandler.configUpdated(config);
	sensorManager.setup();
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
	soundManager.update();
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	sensorManager.update(now);
	SystemCpuMonitor::endTask();

	SystemCpuMonitor::startTask();
	bluetoothController.loop();
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
