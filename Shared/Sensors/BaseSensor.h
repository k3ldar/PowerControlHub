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