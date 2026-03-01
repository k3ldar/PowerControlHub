#pragma once

#include <SensorManager.h>
#include "LoggingSupport.h"
#include "Local.h"

#if defined(FUSE_BOX_CONTROLLER)
#include "JsonVisitor.h"
#endif

enum class SensorType : uint8_t
{
	Local = 0,
	Remote = 1,
};

#if defined(MQTT_SUPPORT)

struct MqttSensorChannel
{
	const char* name;
	const char* slug;
	const char* deviceClass;
	const char* unit;
	bool isBinary;
};

#endif

class BaseSensor : public BaseSensorHandler
#if defined(FUSE_BOX_CONTROLLER)
	, public JsonVisitor
#endif
{
public:
	virtual ~BaseSensor() = default;

	virtual SensorIdList getSensorId() const = 0;

	virtual SensorType getSensorType() const = 0;

	virtual const char* getSensorCommandId() const = 0;

	virtual const char* getSensorName() const = 0;

#if defined(MQTT_SUPPORT)
	virtual uint8_t getMqttChannelCount() const = 0;

	virtual MqttSensorChannel getMqttChannel(uint8_t channelIndex) const = 0;

	virtual void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const = 0;
#endif
};