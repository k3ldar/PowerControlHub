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
#pragma once


#include <dht11.h>

#include "Local.h"
#include "SystemDefinitions.h"
#include "WarningManager.h"
#include "WarningType.h"
#include "BaseSensor.h"
#include "MessageBus.h"

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
	MessageBus* _messageBus;
	SensorCommandHandler* _sensorCommandHandler;
	WarningManager* _warningManager;
	dht11 _dht11Sensor;
	const uint8_t _sensorPin;
	float _humidityOffset;
	float _temperatureOffset;
	float _humidity;
	float _celsius;

#if defined(MQTT_SUPPORT)
	char _slugTemp[32];
	char _slugHumidity[32];
	char _nameTemp[48];
	char _nameHumidity[48];
#endif

protected:
	void initialize() override
	{
		pinMode(_sensorPin, INPUT);
	}

	unsigned long update() override
	{
		sendDebug("Reading DHT11 sensor...", _name);
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

		_humidity = _dht11Sensor.humidity + _humidityOffset;
		_celsius = _dht11Sensor.temperature + _temperatureOffset;

		if (_messageBus)
		{
			_messageBus->publish<TemperatureUpdated>(_celsius);
			_messageBus->publish<HumidityUpdated>(static_cast<uint8_t>(_humidity));
		}

		StringKeyValue params[2];
		strncpy(params[0].key, ValueParamName, sizeof(params[0].key));
		strncpy(params[1].key, "n", sizeof(params[1].key));
		strncpy(params[1].value, _name, sizeof(params[1].value) - 1);
		params[1].value[sizeof(params[1].value) - 1] = '\0';
		snprintf(params[0].value, sizeof(params[0].value), "%.1f", _celsius);
		sendCommand(SensorTemperature, params, 2);
		snprintf(params[0].value, sizeof(params[0].value), "%.1f", _humidity);
		sendCommand(SensorHumidity, params, 2);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setTemperature(_celsius);
			_sensorCommandHandler->setHumidity(static_cast<uint8_t>(_humidity));
		}

		return TempHumidityCheckMs;
	}
public:
	Dht11SensorHandler(MessageBus* messageBus, BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		WarningManager* warningManager, uint8_t sensorPin, float humidityOffset, float temperatureOffset, const char* name = "Dht11")
		: BaseSensor(name), BroadcastLoggerSupport(broadcastManager), _messageBus(messageBus), _sensorCommandHandler(sensorCommandHandler),
		_warningManager(warningManager), _dht11Sensor(), _sensorPin(sensorPin), _humidityOffset(humidityOffset), 
		_temperatureOffset(temperatureOffset), _humidity(0.0f), _celsius(0.0f)
	{
#if defined(MQTT_SUPPORT)
		snprintf(_slugTemp, sizeof(_slugTemp), "%s_temperature", _safeSlug);
		snprintf(_slugHumidity, sizeof(_slugHumidity), "%s_humidity", _safeSlug);
		snprintf(_nameTemp, sizeof(_nameTemp), "%s Temperature", _name);
		snprintf(_nameHumidity, sizeof(_nameHumidity), "%s Humidity", _name);
#endif
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
        // Validate output buffer
		if (!buffer || size == 0)
		{
			return;
		}

		char celsius[8];
		char humidity[8];
		char celsiusOffset[8];
		char humidityOffset[8];
		dtostrf(_celsius, 1, 1, celsius);
		dtostrf(_humidity, 1, 1, humidity);
		dtostrf(_temperatureOffset, 1, 1, celsiusOffset);
		dtostrf(_humidityOffset, 1, 1, humidityOffset);

		int written = snprintf(buffer, size, "\"name\":\"%s\",\"SensorPin\":%u,\"temperature\":%s,\"humidity\":%s,\"humOffset\":%s,\"tempOffset\":%s",
			_name, _sensorPin, celsius, humidity, humidityOffset, celsiusOffset);

		if (written < 0 || (size_t)written >= size)
		{
			sendError("Status JSON truncated", _name);
			buffer[size - 1] = '\0';
		}
	}

	SensorIdList getSensorIdType() const override
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

#if defined(MQTT_SUPPORT)
	uint8_t getMqttChannelCount() const override
	{
		return 2;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		if (channelIndex == 0)
		{
			return { _nameTemp, _slugTemp, "temperature", "temperature", "\xc2\xb0""C", false };
		}

		return { _nameHumidity, _slugHumidity, "humidity", "humidity", "%", false };
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		if (channelIndex == 0)
		{
			dtostrf(_celsius, 1, 1, buffer);
		}
		else
		{
			snprintf(buffer, size, "%u", static_cast<uint8_t>(_humidity));
		}
	}
#endif
};