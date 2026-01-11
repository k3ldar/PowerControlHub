#pragma once

#include <SensorManager.h>
#include "LoggingSupport.h"

#if defined(FUSE_BOX_CONTROLLER)
#include "JsonVisitor.h"
#endif

enum class SensorType : uint8_t
{
	Local = 0,
	Remote = 1,
};

class BaseSensor : public BaseSensorHandler
#if defined(FUSE_BOX_CONTROLLER)
	, public JsonVisitor
#endif
{
public:
	virtual SensorIdList getSensorId() const = 0;

	virtual SensorType getSensorType() const = 0;

	virtual const char* getSensorCommandId() const = 0;

	virtual const char* getSensorName() const = 0;
};