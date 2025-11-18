#pragma once


#include <SensorManager.h>
#include <SerialCommandManager.h>
#include <dht11.h>
#include "StaticElectricConstants.h"

constexpr unsigned long TempHumidityCheckMs = 2500;


/**
 * @brief Sensor handler for DHT11 monitoring.
 *
 * Reads temperature and humidity sensor values, 
 * and reports readings to both link and computer serial connections.
 */
class Dht11SensorHandler : public BaseSensorHandler
{
private:
	SerialCommandManager* _commandMgrLink;
	SerialCommandManager* _commandMgrComputer;
	dht11 _dht11Sensor;
	const uint8_t _sensorPin;

protected:
	void initialize() override
	{
		pinMode(_sensorPin, INPUT);
	};

	unsigned long update() override
	{
		int result = _dht11Sensor.read(_sensorPin);

		if (result != DHTLIB_OK)
		{
			if (_commandMgrComputer)
			{
				_commandMgrLink->sendCommand(WarningAdd, "0x07=1", "");

				_commandMgrComputer->sendDebug(String(result), F("DHT11 Error"));
			}

			return TempHumidityCheckMs;
		}

		if (_commandMgrComputer)
		{
			_commandMgrComputer->sendDebug(String(_dht11Sensor.humidity, 1), F("Humidity"));
			_commandMgrComputer->sendDebug(String(_dht11Sensor.temperature, 1), F("Temperature"));
		}

		_commandMgrLink->sendCommand(WarningAdd, "0x07=0", "");

		float humidity = _dht11Sensor.humidity;
		float tempCelsius = _dht11Sensor.temperature;

		if (_commandMgrLink)
		{
			_commandMgrLink->sendCommand(SensorTemperature, String(tempCelsius, 1));
			_commandMgrLink->sendCommand(SensorHumidity, String(humidity, 0));
		}

		return TempHumidityCheckMs;
	};
public:
	Dht11SensorHandler(SerialCommandManager* commandManagerLink, SerialCommandManager* commandManagerComputer,
		uint8_t sensorPin)
		: _commandMgrLink(commandManagerLink), _commandMgrComputer(commandManagerComputer), _dht11Sensor(), _sensorPin(sensorPin)
	{
	};
};