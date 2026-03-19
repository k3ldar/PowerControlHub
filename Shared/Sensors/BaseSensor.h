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

// Forward declaration to allow SmartFuseBoxApp to be declared friend below
class SmartFuseBoxApp;

#if defined(FUSE_BOX_CONTROLLER)
#include "JsonVisitor.h"
#endif

enum class SensorType : uint8_t
{
	Local = 0,
	Remote = 1,
};

#if defined(MQTT_SUPPORT)

constexpr uint8_t SafeSlugLength = 20;

struct MqttSensorChannel
{
	const char* name;
	const char* slug;
	const char* typeSlug; 
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
    // Allow top-level application class to access BaseSensor internals
	friend class SmartFuseBoxApp;
private:
	uint8_t _uniqueId = 0;

	void setUniqueId(uint8_t id)
	{
		_uniqueId = id;
	}
protected:
	const char* _name;

#if defined(MQTT_SUPPORT)
	char _safeSlug[SafeSlugLength];

	static void buildSafeSlug(const char* name, char* out, size_t size)
	{
		size_t i = 0;

		while (name[i] && i < size - 1)
		{
			char c = name[i];
			out[i] = (c == ' ') ? '_' : ((c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c);
			i++;
		}

		out[i] = '\0';
	}
#endif

	explicit BaseSensor(const char* name)
		: _name(name)
	{
#if defined(MQTT_SUPPORT)
		buildSafeSlug(_name, _safeSlug, sizeof(_safeSlug));
#endif
	}

public:
	virtual ~BaseSensor() = default;

	uint8_t getUid() const
	{
		return _uniqueId;
	}

	virtual SensorIdList getSensorIdType() const = 0;

	virtual SensorType getSensorType() const = 0;

	virtual const char* getSensorCommandId() const = 0;

	virtual const char* getSensorName() const
	{
		return _name;
	}

#if defined(MQTT_SUPPORT)
	virtual uint8_t getMqttChannelCount() const = 0;

	virtual MqttSensorChannel getMqttChannel(uint8_t channelIndex) const = 0;

	virtual void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const = 0;

	virtual unsigned long getMqttPublishIntervalMs(uint8_t channelIndex) const
	{
		(void)channelIndex;
		return 1000; // Default: publish every 1 second
	}
#endif
};