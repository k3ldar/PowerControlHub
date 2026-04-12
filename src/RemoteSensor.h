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
#include "BaseSensor.h"
#include "MessageBus.h"
#include "SensorCommandHandler.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"

constexpr uint8_t MaxRemoteParams = 5;
constexpr uint64_t RemoteSensorDefaultUpdateIntervalMs = 60000; // 60s

// Generic remote sensor base - listens for MqttMessageReceived events and
// receives updates from remote sensors via SensorCommandHandler.
class RemoteSensor : public BaseSensor
{
private:
	const SensorIdList _sensorId;
    const char* _sensorName;
    const char* _commandId;
    StringKeyValue* _remoteParamsStorage;

#if defined(MQTT_SUPPORT)
    const char* _mqttTopic;
    const MqttSensorChannel* _mqttChannels;
#endif
    uint8_t _paramCount;

public:
    RemoteSensor(SensorIdList sensorId, const char* sensorName, const char* commandId,
#if defined(MQTT_SUPPORT)
        const char* mqttTopic, const MqttSensorChannel* mqttChannels,
#endif
        uint8_t paramCount)
        : BaseSensor(sensorName), _sensorId(sensorId), _sensorName(sensorName), _commandId(commandId),
#if defined(MQTT_SUPPORT)
          _mqttTopic(mqttTopic), _mqttChannels(mqttChannels),
#endif
          _paramCount(paramCount)
    {
        if (_paramCount > MaxRemoteParams)
            _paramCount = MaxRemoteParams;

        if (_paramCount == 0)
        {
            _remoteParamsStorage = nullptr;
        
		}
        else
        {
            _remoteParamsStorage = new StringKeyValue[_paramCount];

            for (uint8_t i = 0; i < _paramCount; i++)
            {
                strncpy(_remoteParamsStorage[i].key, "no name", sizeof(_remoteParamsStorage[i].key));
                _remoteParamsStorage[i].key[sizeof(_remoteParamsStorage[i].key) - 1] = '\0';

                strncpy(_remoteParamsStorage[i].value, "0", sizeof(_remoteParamsStorage[i].value));
                _remoteParamsStorage[i].value[sizeof(_remoteParamsStorage[i].value) - 1] = '\0';
            }
        }
    }

    ~RemoteSensor()
    {
        delete[] _remoteParamsStorage;
		_remoteParamsStorage = nullptr;
	}

    RemoteSensor(const RemoteSensor&) = delete;
    RemoteSensor& operator=(const RemoteSensor&) = delete;

    void initialize() override
    {
    }

    uint64_t update() override
    {
		// no need for polling, updates are triggered by SensorCommandHandler 
        // when a command is received, so just return a long delay here
        return RemoteSensorDefaultUpdateIntervalMs;
    }

    SensorType getSensorType() const override
    {
        return SensorType::Remote;
    }

    const char* getSensorCommandId() const override
    {
        return _commandId;
    }

    // Default name
    const char* getSensorName() const override
    {
        return _sensorName;
    }

#if defined(MQTT_SUPPORT)
    uint8_t getMqttChannelCount() const override
    {
        return _paramCount;
    }

    MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
    {
        if (!_mqttChannels || channelIndex >= _paramCount)
			return { nullptr, nullptr, nullptr, nullptr, nullptr, false };

		return _mqttChannels[channelIndex];
    }

    void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
    {
        if (!_mqttChannels || channelIndex >= _paramCount || !_remoteParamsStorage)
        {
            snprintf(buffer, size, "0");
            return;
        }

        if (channelIndex >= MaxRemoteParams)
        {
            snprintf(buffer, size, "0");
            return;
        }

        snprintf(buffer, size, "%s", _remoteParamsStorage[channelIndex].value);
    }
#endif

    SensorIdList getSensorIdType() const override
    {
        return _sensorId;
    }

    void formatStatusJson(char* buffer, size_t size) override
    {
        if (!buffer || size == 0 || !_remoteParamsStorage)
        {
            if (buffer && size > 0)
                buffer[0] = '\0';

            return;
        }

        int pos = 0;
        
        // open object
        int written = snprintf(buffer, size, "\"%s\":{", _sensorName);

        if (written < 0 || (size_t)written >= size)
            return;

        pos += written;

        for (uint8_t i = 0; i < _paramCount; i++)
        {
            if (i > 0)
            {
                written = snprintf(buffer + pos, size - pos, ",");

                if (written < 0 || (size_t)written >= size - pos)
                    return;

                pos += written;
            }

            written = snprintf(buffer + pos, size - pos, "\"%s\":\"%s\"",
                _remoteParamsStorage[i].key, _remoteParamsStorage[i].value);

            if (written < 0 || (size_t)written >= size - pos)
                return;

            pos += written;
        }

        // close object
        snprintf(buffer + pos, size - pos, "}");
    }

    void handleRemoteCommand(const StringKeyValue params[], uint8_t paramCount)
    {
        if (paramCount == 0 || paramCount != _paramCount || !_remoteParamsStorage)
            return;

        for (uint8_t i = 0; i < _paramCount; i++)
        {
            // safe copy of key and value
            strncpy(_remoteParamsStorage[i].key, params[i].key, sizeof(_remoteParamsStorage[i].key) - 1);
            _remoteParamsStorage[i].key[sizeof(_remoteParamsStorage[i].key) - 1] = '\0';

            strncpy(_remoteParamsStorage[i].value, params[i].value, sizeof(_remoteParamsStorage[i].value) - 1);
            _remoteParamsStorage[i].value[sizeof(_remoteParamsStorage[i].value) - 1] = '\0';
        }
    }
};
