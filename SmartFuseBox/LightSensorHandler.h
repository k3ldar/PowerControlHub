#pragma once

#include "Local.h"
#include "BaseSensor.h"

constexpr unsigned long LightCheckMs = 30000;

class LightSensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
	MessageBus* _messageBus;
	SensorCommandHandler* _sensorCommandHandler;
	WarningManager* _warningManager;
	const uint8_t _sensorPin;
	const uint8_t _analogPin;
	bool _isPinActive;
	bool _isDaytime;
protected:
	void initialize() override
	{
		pinMode(_sensorPin, INPUT);
	}

	unsigned long update() override
	{
		_isDaytime = digitalRead(_sensorPin) == LOW;

		if (_messageBus)
		{
			_messageBus->publish<LightSensorUpdated>(_isDaytime);
		}

		StringKeyValue params;
		strncpy(params.key, ValueParamName, sizeof(params.key));
		snprintf(params.value, sizeof(params.value), "%d", _isDaytime);
		sendCommand(SensorLightSensor, &params, 1);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setDaytime(_isDaytime);
		}

		return LightCheckMs;
	}
public:
	LightSensorHandler(MessageBus* messageBus, BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		WarningManager* warningManager, uint8_t sensorPin, uint8_t analogPin)
		: BroadcastLoggerSupport(broadcastManager), _messageBus(messageBus), _sensorCommandHandler(sensorCommandHandler),
		_warningManager(warningManager), _sensorPin(sensorPin), _analogPin(analogPin), _isPinActive(false), _isDaytime(true)
	{
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
		snprintf(buffer, size, "\"isDaytime\":%s", _isDaytime ? "true" : "false");
	}

	SensorIdList getSensorId() const override
	{
		return SensorIdList::LightSensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SensorLightSensor;
	}

	const char* getSensorName() const override
	{
		return "LightSensor";
	}

#if defined(MQTT_SUPPORT)

	uint8_t getMqttChannelCount() const override
	{
		return 1;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		(void)channelIndex;
		return { "Daylight", "light", "light", nullptr, true };
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		(void)channelIndex;
		snprintf(buffer, size, "%s", _isDaytime ? "ON" : "OFF");
	}
#endif
};