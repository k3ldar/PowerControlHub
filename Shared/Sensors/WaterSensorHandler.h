#pragma once


#include "Queue.h"
#include "SmartFuseBoxConstants.h"
#include "WarningType.h"
#include "LoggingSupport.h"
#include "JsonVisitor.h"
#include "BaseSensor.h"

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
	SensorCommandHandler* _sensorCommandHandler;
	const uint8_t _sensorPin;
	const uint8_t _activePin;
	Queue<uint16_t> _waterPumpQueue;
	uint16_t _latestWaterLevel;
	bool _waitingForStabilization;

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

		StringKeyValue params[] = {
			{"avg", String(_waterPumpQueue.average())},
			{"v", String(sensorValue) }
		};

		sendCommand(SensorWaterLevel, params, 2);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setWaterLevel(static_cast<uint16_t>(_waterPumpQueue.average()));
		}

		sendDebug(String(sensorValue), F("WTRLVL"));
		sendDebug(String(_waterPumpQueue.average()), F("WTRAVG"));

		return WaterSensorCheckIntervalMs;
	}
public:
	WaterSensorHandler(BroadcastManager* broadcastManager,
		SensorCommandHandler* sensorCommandHandler, uint8_t sensorPin, uint8_t activePin)
		: BroadcastLoggerSupport(broadcastManager), _sensorCommandHandler(sensorCommandHandler),
			_sensorPin(sensorPin), _activePin(activePin), _waterPumpQueue(15, 0),
		_latestWaterLevel(0), _waitingForStabilization(false)
	{
		pinMode(sensorPin, INPUT);
		digitalWrite(_activePin, LOW);
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
		snprintf(buffer, size, "\"level\":%d,\"average\":%d",
			_latestWaterLevel, _waterPumpQueue.average());
	}

	SensorIdList getSensorId() const override
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

	const char* getSensorName() const override
	{
		return "waterLevel";
	}
};