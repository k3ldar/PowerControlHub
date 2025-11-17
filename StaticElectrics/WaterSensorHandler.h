#pragma once


#include "SensorManager.h"
#include "Queue.h"
#include <SerialCommandManager.h>
#include "StaticElectricConstants.h"
constexpr unsigned long WaterSensorCheckIntervalMs = 5000;
constexpr unsigned long WaterSensorStabilizeMs = 10;

/**
 * @brief Sensor handler for water level monitoring with averaging.
 *
 * Reads analog water sensor values, maintains a rolling average using a queue,
 * and reports readings to both link and computer serial connections.
 */
class WaterSensorHandler : public BaseSensorHandler
{
private:
	SerialCommandManager* _commandMgrLink;
	SerialCommandManager* _commandMgrComputer;
	const uint8_t _sensorPin;
	const uint8_t _activePin;
	Queue _waterPumpQueue;
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
		int sensorValue = analogRead(WaterSensorPin);
		_waterPumpQueue.enqueue(sensorValue);

		digitalWrite(WaterSensorActivePin, LOW);


		StringKeyValue params[] = {
			{"avg", String(_waterPumpQueue.average())},
			{"v", String(sensorValue) }
		};

		if (_commandMgrLink)
		{
			_commandMgrLink->sendCommand(SensorWaterLevel, "", "", params, 2);
		}

		if (_commandMgrComputer)
		{
			_commandMgrComputer->sendDebug(String(sensorValue), F("WTRLVL"));
			_commandMgrComputer->sendDebug(String(_waterPumpQueue.average()), F("WTRAVG"));
		}

		return WaterSensorCheckIntervalMs;
	};
public:
	WaterSensorHandler(SerialCommandManager* commandManagerLink, SerialCommandManager* commandManagerComputer, 
		uint8_t sensorPin, uint8_t activePin)
		: _commandMgrLink(commandManagerLink), _commandMgrComputer(commandManagerComputer), _sensorPin(sensorPin), _activePin(activePin),
			_waterPumpQueue(15), _waitingForStabilization(false)
	{
		pinMode(sensorPin, INPUT);
		digitalWrite(_activePin, LOW);
	};
};