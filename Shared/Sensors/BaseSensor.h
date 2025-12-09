#pragma once

#include <SensorManager.h>
#include "LoggingSupport.h"
#include "JsonVisitor.h"

enum class SensorType : uint8_t
{
	Local = 0,
	Remote = 1,
};

class BaseSensor : public BaseSensorHandler, public JsonVisitor
{
public:
	virtual SensorIdList getSensorId() const = 0;

	virtual SensorType getSensorType() const = 0;

	virtual const char* getSensorCommandId() const = 0;

	virtual const char* getSensorName() const = 0;
};