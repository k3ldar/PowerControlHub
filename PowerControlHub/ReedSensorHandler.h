/*
 * PowerControlHub
 * Copyright (C) 2026 Simon Carter (s1cart3r@gmail.com)
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

#include <Arduino.h>

#include "Local.h"
#include "SystemDefinitions.h"
#include "BaseSensor.h"
#include "MessageBus.h"

constexpr uint64_t ReedCheckMs = 300;
constexpr uint8_t ReedContactNormallyOpen = 0;
constexpr uint8_t ReedContactNormallyClosed = 1;

/**
 * @brief Sensor handler for a magnetic reed contact switch.
 *
 * Reads a digital GPIO pin connected to a reed switch and reports
 * open/closed state to the local UI, MQTT and the message bus.
 * Optionally drives an output pin high while the contact is active.
 */
class ReedSensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
    MessageBus* _messageBus;
    const uint8_t _sensorPin;
    bool _isNormallyOpen;
    uint8_t _activePin;
    bool _active;

#if defined(MQTT_SUPPORT)
    char _slugContact[36];
    char _nameContact[48];
#endif

    bool readActive() const
    {
        int level = digitalRead(_sensorPin);
        // Pin is pulled up and the contact ties it to GND: closed = LOW, open = HIGH.
        // NO activates when closed (LOW); NC activates when open (HIGH).
        return _isNormallyOpen ? level == LOW : level == HIGH;
    }

    void applyActivePin()
    {
        if (_activePin < PinDisabled)
            digitalWrite(_activePin, _active ? HIGH : LOW);
    }

    const char* stateText() const
    {
        return _active ? "detected" : "clear";
    }

protected:
    void initialize() override
    {
        pinMode(_sensorPin, INPUT_PULLUP);

        if (_activePin < PinDisabled)
        {
            pinMode(_activePin, OUTPUT);
            digitalWrite(_activePin, LOW);
        }

        _active = readActive();
        applyActivePin();

        char buf[48];
        snprintf(buf, sizeof(buf), "pin=%u activePin=%u state=%s",
            _sensorPin, _activePin, stateText());
        sendDebug(buf, _name);
    }

    uint64_t update() override
    {
        bool active = readActive();

        if (active != _active)
        {
            _active = active;
            applyActivePin();

            StringKeyValue params[2];
            strncpy(params[0].key, ValueParamName, sizeof(params[0].key));
            snprintf(params[0].value, sizeof(params[0].value), "%d", _active ? 1 : 0);
            strncpy(params[1].key, "name", sizeof(params[1].key));
            strncpy(params[1].value, _name, sizeof(params[1].value));
            sendCommand(SensorReed, params, 2);

            if (_messageBus)
                _messageBus->publish<BinaryPresenceUpdated>(_active, _name);

            char buf[40];
            snprintf(buf, sizeof(buf), "State -> %s", stateText());
            sendDebug(buf, _name);
        }

        return ReedCheckMs;
    }

public:
    ReedSensorHandler(MessageBus* messageBus, BroadcastManager* broadcastManager,
        uint8_t sensorPin, bool normallyOpen, uint8_t activePin, const char* name = "ReedSensor")
        : BaseSensor(name), BroadcastLoggerSupport(broadcastManager),
        _messageBus(messageBus), _sensorPin(sensorPin), _isNormallyOpen(normallyOpen),
        _activePin(activePin), _active(false)
    {
#if defined(MQTT_SUPPORT)
        snprintf(_slugContact, sizeof(_slugContact), "%s_contact", _safeSlug);
        snprintf(_nameContact, sizeof(_nameContact), "%s Contact", _name);
#endif
    }

    void formatStatusJson(char* buffer, size_t size) override
    {
        if (!buffer || size == 0)
            return;

        int written = snprintf(buffer, size, "\"SensorPin\":%u,\"ActivePin\":%u,\"state\":\"%s\"",
            _sensorPin, _activePin, stateText());

        if (written < 0 || (size_t)written >= size)
        {
            sendError("Status JSON truncated", _name);
            buffer[size - 1] = '\0';
        }
    }

    SensorIdList getSensorIdType() const override
    {
        return SensorIdList::ReedSensor;
    }

    SensorType getSensorType() const override
    {
        return SensorType::Local;
    }

    const char* getSensorCommandId() const override
    {
        return SensorReed;
    }

#if defined(MQTT_SUPPORT)
    uint8_t getMqttChannelCount() const override
    {
        return 1;
    }

    MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
    {
        (void)channelIndex;
        return { _nameContact, _slugContact, "binary_sensor", "door", nullptr, true };
    }

    void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
    {
        (void)channelIndex;

        if (!buffer || size == 0)
            return;

        snprintf(buffer, size, "%s", _active ? "ON" : "OFF");
    }
#endif
};