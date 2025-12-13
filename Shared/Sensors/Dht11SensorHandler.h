#pragma once


#include <dht11.h>

#include "SystemDefinitions.h"
#include "WarningManager.h"
#include "WarningType.h"
#include "BaseSensor.h"

constexpr unsigned long TempHumidityCheckMs = 2500;

/**
 * @brief Sensor handler for DHT11 monitoring.
 *
 * Reads temperature and humidity sensor values, 
 * and reports readings to both link and computer serial connections.
 */
class Dht11SensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
	SensorCommandHandler* _sensorCommandHandler;
	WarningManager* _warningManager;
	dht11 _dht11Sensor;
	const uint8_t _sensorPin;
	float _humidity;
	float _celsius;

protected:
	void initialize() override
	{
		pinMode(_sensorPin, INPUT);
	}

	unsigned long update() override
	{
		uint16_t result = _dht11Sensor.read(_sensorPin);

		if (result != DHTLIB_OK)
		{
			if (_warningManager && !_warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
			{
				_warningManager->raiseWarning(WarningType::TemperatureSensorFailure);
				char buffer[32];
				snprintf(buffer, sizeof(buffer), "DHT11 read error: %u", result);
				sendError(buffer, "DHT11 Error");
			}

			return TempHumidityCheckMs;
		}

		if (_warningManager && _warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
		{
			_warningManager->clearWarning(WarningType::TemperatureSensorFailure);
		}

		_humidity = _dht11Sensor.humidity;
		_celsius = _dht11Sensor.temperature;

		StringKeyValue params;
		strcpy(params.key, ValueParamName);
		snprintf(params.value, sizeof(params.value), "%f3.1", _celsius);
		sendCommand(SensorTemperature, &params, 1);
		snprintf(params.value, sizeof(params.value), "%f3.1", _humidity);
		sendCommand(SensorHumidity, &params, 1);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setTemperature(_celsius);
			_sensorCommandHandler->setHumidity(static_cast<uint8_t>(_humidity));
		}

		return TempHumidityCheckMs;
	}
public:
	Dht11SensorHandler(BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		WarningManager* warningManager, uint8_t sensorPin)
		: BroadcastLoggerSupport(broadcastManager), _sensorCommandHandler(sensorCommandHandler),
			_warningManager(warningManager), _dht11Sensor(), _sensorPin(sensorPin), _humidity(0.0f), _celsius(0.0f)
	{
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
		char celsius[8];
		char humidity[8];
		dtostrf(_celsius, 6, 1, celsius);
		dtostrf(_humidity, 6, 1, humidity);
		snprintf(buffer, size, "\"temperature\":%s,\"humidity\":%s",
			celsius, humidity);
	}

	SensorIdList getSensorId() const override
	{
		return SensorIdList::Dht11Sensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SensorTemperature;
	}

	const char* getSensorName() const override
	{
		return "dht11";
	}
};