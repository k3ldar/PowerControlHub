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

#include "Local.h"
#include "Queue.h"
#include "SmartFuseBoxConstants.h"
#include "WarningType.h"
#include "LoggingSupport.h"
#include "JsonVisitor.h"
#include "BaseSensor.h"
#include "MessageBus.h"

constexpr unsigned long WaterSensorCheckIntervalMs = 5000;
constexpr unsigned long WaterSensorStabilizeMs = 10;

/**
 * @brief Sensor handler for water level monitoring with averaging.
 *
 * Reads analog water sensor values, maintains a rolling average using a queue,
 * and reports readings to both link and computer serial connections.
 */
class WaterSensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
	MessageBus* _messageBus;
	SensorCommandHandler* _sensorCommandHandler;
	const uint8_t _sensorPin;
	const uint8_t _activePin;
	Queue<uint16_t> _waterPumpQueue;
	uint16_t _latestWaterLevel;
	bool _waitingForStabilization;

#if defined(MQTT_SUPPORT)
	char _slugWaterLevel[32];
	char _nameWaterLevel[48];
#endif

protected:
	void initialize() override
	{
	};

	unsigned long update() override
	{
		if (_waterPumpQueue.isFull())
		{
			_waterPumpQueue.dequeue();
		}

		if (!_waitingForStabilization)
		{
			digitalWrite(WaterSensorActivePin, HIGH);
			_waitingForStabilization = true;
			return WaterSensorStabilizeMs;
		}

		_waitingForStabilization = false;
		int sensorValue = analogRead(_sensorPin);
		_latestWaterLevel = static_cast<uint16_t>(sensorValue);
		_waterPumpQueue.enqueue(_latestWaterLevel);

		digitalWrite(WaterSensorActivePin, LOW);

		StringKeyValue params[2];
		strncpy(params[0].key, "avg", sizeof(params[0].key));
		snprintf(params[0].value, sizeof(params[0].value), "%u", static_cast<unsigned int>(_waterPumpQueue.average()));
		strncpy(params[1].key, "v", sizeof(params[1].key));
		snprintf(params[1].value, sizeof(params[1].value), "%u", static_cast<unsigned int>(sensorValue));

		if (_messageBus)
		{
			_messageBus->publish<WaterLevelUpdated>(_latestWaterLevel, _waterPumpQueue.average());
		}
		sendCommand(SensorWaterLevel, params, 2);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setWaterLevel(static_cast<uint16_t>(_waterPumpQueue.average()));
		}

		return WaterSensorCheckIntervalMs;
	}
public:
	WaterSensorHandler(MessageBus* messageBus, BroadcastManager* broadcastManager,
		SensorCommandHandler* sensorCommandHandler, uint8_t sensorPin, uint8_t activePin, const char* name = "WaterLevel")
		: BaseSensor(name), BroadcastLoggerSupport(broadcastManager), _messageBus(messageBus), _sensorCommandHandler(sensorCommandHandler),
			_sensorPin(sensorPin), _activePin(activePin), _waterPumpQueue(15, 0),
			_latestWaterLevel(0), _waitingForStabilization(false)
	{
		pinMode(sensorPin, INPUT);
		digitalWrite(_activePin, LOW);

#if defined(MQTT_SUPPORT)
		snprintf(_slugWaterLevel, sizeof(_slugWaterLevel), "%s_water_level", _safeSlug);
		snprintf(_nameWaterLevel, sizeof(_nameWaterLevel), "%s Water Level", _name);
#endif
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
		snprintf(buffer, size, "\"level\":%d,\"average\":%d",
			_latestWaterLevel, _waterPumpQueue.average());
	}

	SensorIdList getSensorIdType() const override
	{
		return SensorIdList::WaterSensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SensorWaterLevel;
	}

#if defined(MQTT_SUPPORT)

	uint8_t getMqttChannelCount() const override
	{
		return 1;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		(void)channelIndex;
		return { _nameWaterLevel, _slugWaterLevel, "water_level", nullptr, nullptr, false };
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		(void)channelIndex;
		snprintf(buffer, size, "%u", static_cast<unsigned int>(_waterPumpQueue.average()));
	}
#endif
};