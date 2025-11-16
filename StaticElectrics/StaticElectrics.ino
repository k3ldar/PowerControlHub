#include <Arduino.h>
#include <stdint.h>
#include <UnoWiFiDevEd.h>
#include <Adafruit_Sensor.h>
#include <dht11.h>
#include <SerialCommandManager.h>

#include "StaticElectricConstants.h"
#include "Config.h"
#include "ConfigManager.h"
#include "ConfigCommandHandler.h"
#include "Queue.h"
#include "SoundManager.h"
#include "SoundCommandHandler.h"
#include "RelayCommandHandler.h"
#include "BaseCommandHandler.h"


#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

constexpr int DefaultDelay = 5;

constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t WaterSensorActivePin = D8;

constexpr unsigned long SensorCheckIntervalMs = 5000;

constexpr unsigned long serialInitTimeoutMs = 300;
constexpr unsigned long serialReconnectMs = 10000;

constexpr uint8_t TempSensorPin = D9;

// forward declares
void InitializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection = false);
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', '=', 500, 64);

SoundManager soundManager;

RelayCommandHandler relayHandler(&commandMgrComputer, &commandMgrLink, Relays, TotalRelays);
SoundCommandHandler soundHandler(&commandMgrComputer, &commandMgrLink, &soundManager);
ConfigCommandHandler configHandler(&soundManager);

unsigned long nextWaterSensorCheck = 5000;
Queue waterPumpQueue(15);

dht11 dht11Sensor;

unsigned long nextWeatherSensorCheck = 2500;
unsigned long lastSerialConnectAttempt = 0;


void setup()
{
	ISerialCommandHandler* linkHandlers[] = { &relayHandler, &soundHandler, &configHandler } ;
	size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
	commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

	ISerialCommandHandler* computerHandlers[] = { &relayHandler, &soundHandler, &configHandler };
	size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
	commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

	InitializeSerial(COMPUTER_SERIAL, 115200, true);
	InitializeSerial(LINK_SERIAL, 9600, true);

	soundManager.configUpdated(ConfigManager::getConfigPtr());

	// water sensor
	pinMode(WaterSensorActivePin, OUTPUT);
	digitalWrite(WaterSensorActivePin, LOW);
	relayHandler.setup();

	commandMgrComputer.sendCommand(SystemInitialized, "");
}

void loop() 
{
	unsigned long now = millis();
	commandMgrComputer.readCommands();
	commandMgrLink.readCommands();
	soundManager.update();

	getWaterSensorValue(now);
	readDHT11Sensor(now);

	delay(DefaultDelay);
}

void getWaterSensorValue(unsigned long currTime)
{
	if (currTime > nextWaterSensorCheck)
	{
		if (waterPumpQueue.isFull())
		  waterPumpQueue.dequeue();

		digitalWrite(WaterSensorActivePin, HIGH);

		delay(10);
		int sensorValue = analogRead(WaterSensorPin);
		waterPumpQueue.enqueue(sensorValue);

		digitalWrite(WaterSensorActivePin, LOW);

		nextWaterSensorCheck = currTime + SensorCheckIntervalMs;
		commandMgrLink.sendCommand(SensorWaterLevel, String(waterPumpQueue.average()));

		commandMgrComputer.sendDebug(String(sensorValue), F("WTRLVL"));
		commandMgrComputer.sendDebug(String(waterPumpQueue.average()), F("WTRAVG"));
	}
}

void readDHT11Sensor(unsigned long currTime)
{
	if (currTime > nextWeatherSensorCheck)
	{
		dht11Sensor.read(TempSensorPin);
		commandMgrComputer.sendDebug(String(dht11Sensor.humidity, 1), F("Humidity"));
		commandMgrComputer.sendDebug(String(dht11Sensor.temperature, 1), F("Temperature"));
		float humidity = dht11Sensor.humidity;
		float tempCelsius = dht11Sensor.temperature;

		nextWeatherSensorCheck = currTime + SensorCheckIntervalMs;

		commandMgrLink.sendCommand(SensorTemperature, String(tempCelsius, 1));
		commandMgrLink.sendCommand(SensorHumidity, String(humidity, 0));
	}
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
