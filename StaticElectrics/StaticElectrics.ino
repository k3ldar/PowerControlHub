#include <Arduino.h>
#include <stdint.h>
#include <SerialCommandManager.h>

#include "StaticElectricConstants.h"
#include "Config.h"
#include "ConfigManager.h"
#include "ConfigCommandHandler.h"
#include "SoundManager.h"

#include "SystemCommandHandler.h"
#include "AckCommandHandler.h"
#include "SoundCommandHandler.h"
#include "RelayCommandHandler.h"
#include "BaseCommandHandler.h"

#include "WaterSensorHandler.h"
#include "Dht11SensorHandler.h"
#include "SensorManager.h"


#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

// forward declares
void InitializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection = false);
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', '=', 500, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

SoundManager soundManager;

SystemCommandHandler systemHandler(&broadcastManager);
AckCommandHandler ackHandler(&broadcastManager);
RelayCommandHandler relayHandler(&commandMgrComputer, &commandMgrLink, Relays, TotalRelays);
SoundCommandHandler soundHandler(&commandMgrComputer, &commandMgrLink, &soundManager);
ConfigCommandHandler configHandler(&soundManager);


// Sensors
WaterSensorHandler waterSensorHandler(&commandMgrLink, &commandMgrComputer, WaterSensorPin, WaterSensorActivePin);
Dht11SensorHandler dht11SensorHandler(&commandMgrLink, &commandMgrComputer, Dht11SensorPin);

BaseSensorHandler* sensorHandlers[] = {
	&waterSensorHandler, &dht11SensorHandler
};
SensorManager sensorManager(sensorHandlers, sizeof(sensorHandlers) / sizeof(sensorHandlers[0]));


void setup()
{
	ISerialCommandHandler* linkHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemHandler } ;
	size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
	commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

	ISerialCommandHandler* computerHandlers[] = { &relayHandler, &soundHandler, &configHandler, &ackHandler, &systemHandler };
	size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
	commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

	InitializeSerial(COMPUTER_SERIAL, 115200, true);
	InitializeSerial(LINK_SERIAL, 9600, true);

	sensorManager.setup();
	relayHandler.setup();

	soundManager.configUpdated(ConfigManager::getConfigPtr());

	commandMgrComputer.sendCommand(SystemInitialized, "");
}

void loop() 
{
	unsigned long now = millis();
	commandMgrComputer.readCommands();
	commandMgrLink.readCommands();
	soundManager.update();
	sensorManager.update(now);

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

void InitializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection)
{
	serialPort.begin(baudRate);

	if (waitForConnection)
	{
		unsigned long leave = millis() + SerialInitTimeoutMs;

		while (!serialPort && millis() < leave)
			delay(10);

		if (serialPort)
			delay(100);
	}
}
