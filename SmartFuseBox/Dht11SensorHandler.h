#pragma once


#include <SensorManager.h>
#include <SerialCommandManager.h>
#include <dht11.h>
#include "SharedConstants.h"
#include "WarningManager.h"
#include "WarningType.h"

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
	WarningManager* _warningManager;
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
				if (_warningManager && !_warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
				{
					_warningManager->raiseWarning(WarningType::TemperatureSensorFailure);
				}

				_commandMgrComputer->sendDebug(String(result), F("DHT11 Error"));
			}

			return TempHumidityCheckMs;
		}

		if (_commandMgrComputer)
		{
			_commandMgrComputer->sendDebug(String(_dht11Sensor.humidity, 1), F("Humidity"));
			_commandMgrComputer->sendDebug(String(_dht11Sensor.temperature, 1), F("Temperature"));
		}

		if (_warningManager && _warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
		{
			_warningManager->clearWarning(WarningType::TemperatureSensorFailure);
		}

		float humidity = _dht11Sensor.humidity;
		float tempCelsius = _dht11Sensor.temperature;

		if (_commandMgrLink)
		{
			StringKeyValue params[] = { { "v", String(tempCelsius, 1) } };
			_commandMgrLink->sendCommand(SensorTemperature, "", "", params, 1);
			params->value = String(humidity, 0);
			_commandMgrLink->sendCommand(SensorHumidity, "", "", params, 1);
		}

		return TempHumidityCheckMs;
	};
public:
	Dht11SensorHandler(SerialCommandManager* commandManagerLink, SerialCommandManager* commandManagerComputer, 
		WarningManager* warningManager, uint8_t sensorPin)
		: _commandMgrLink(commandManagerLink), _commandMgrComputer(commandManagerComputer), _warningManager(warningManager), _dht11Sensor(), _sensorPin(sensorPin)
	{
	};
};