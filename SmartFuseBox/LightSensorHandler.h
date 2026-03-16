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
#include "Queue.h"
#include "RelayController.h"

constexpr unsigned long LightCheckMs = 30000;
constexpr uint8_t LightQueueCapacity = 10;
constexpr uint8_t DaytimeConfirmReadings = 3;
constexpr uint8_t AnalogHysteresisPct = 10;

class LightSensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
    MessageBus* _messageBus;
    SensorCommandHandler* _sensorCommandHandler;
    RelayController* _relayController;
    const uint8_t _sensorPin;
    bool _isDigital;
    const char* _name;
    bool _isDaytime;
    bool _pendingDaytime;
    uint8_t _pendingDaytimeCount;
    Queue<uint16_t> _lightQueue;
    uint16_t _currentLightLevel;

    void syncNightRelay(Config* config)
    {
        if (config == nullptr || _relayController == nullptr)
            return;

        uint8_t nightRelay = config->lightSensor.nightRelayIndex;

        if (nightRelay < ConfigRelayCount)
        {
            char buf[40];
            snprintf(buf, sizeof(buf), "relay=%u state=%s", nightRelay, !_isDaytime ? "on" : "off");
            sendDebug(buf, _name);
            _relayController->setRelayState(nightRelay, !_isDaytime);
        }
        else
        {
            char buf[40];
            snprintf(buf, sizeof(buf), "relay=%u skipped (disabled)", nightRelay);
            sendDebug(buf, _name);
        }
    }
    uint16_t effectiveAverage() const
    {
        return _isDigital ? _currentLightLevel : _lightQueue.average();
    }
protected:
    void initialize() override
    {
        pinMode(_sensorPin, INPUT);
        sendDebug(_isDigital ? "Mode: digital" : "Mode: analog", _name);
        syncNightRelay(ConfigManager::getConfigPtr());
    }

    unsigned long update() override
    {
        Config* config = ConfigManager::getConfigPtr();
        bool reading;

        if (_isDigital)
        {
            reading = digitalRead(_sensorPin) == HIGH;
            _currentLightLevel = reading ? 1023 : 0;

            char buf[32];
            snprintf(buf, sizeof(buf), "pin=%d state=%s day=%d",
                _sensorPin, reading ? "HIGH" : "LOW", _isDaytime ? 1 : 0);
            sendDebug(buf, _name);
        }
        else
        {
            _currentLightLevel = static_cast<uint16_t>(analogRead(_sensorPin));
            _lightQueue.enqueue(_currentLightLevel);

            uint16_t avgLevel = _lightQueue.average();
            uint16_t threshold = (config != nullptr) ? config->lightSensor.daytimeThreshold : 512;
            uint16_t hysteresis = threshold / AnalogHysteresisPct;
            if (hysteresis == 0) hysteresis = 1;
            uint16_t lower = (threshold > hysteresis) ? threshold - hysteresis : 0;
            uint16_t upper = (threshold + hysteresis > 1023) ? 1023 : threshold + hysteresis;
            reading = _isDaytime ? (avgLevel > lower) : (avgLevel > upper);

            char buf[64];
            snprintf(buf, sizeof(buf), "level=%u avg=%u lo=%u hi=%u day=%d",
                _currentLightLevel, avgLevel, lower, upper, _isDaytime ? 1 : 0);
            sendDebug(buf, _name);
        }

        if (reading == _pendingDaytime)
        {
            if (_pendingDaytimeCount < DaytimeConfirmReadings)
                _pendingDaytimeCount++;
        }
        else
        {
            char buf[40];
            snprintf(buf, sizeof(buf), "Pending flip: %s count reset", reading ? "day" : "night");
            sendDebug(buf, _name);
            _pendingDaytime = reading;
            _pendingDaytimeCount = 1;
        }

        if (_pendingDaytimeCount >= DaytimeConfirmReadings && _isDaytime != _pendingDaytime)
        {
            _isDaytime = _pendingDaytime;

            char buf[24];
            snprintf(buf, sizeof(buf), "State -> %s", _isDaytime ? "day" : "night");
            sendDebug(buf, _name);
            syncNightRelay(config);
        }

        if (_messageBus)
        {
            _messageBus->publish<LightSensorUpdated>(_isDaytime, _currentLightLevel, effectiveAverage());
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
        RelayController* relayController, uint8_t sensorPin, bool isDigital, const char* name = "LightSensor")
        : BroadcastLoggerSupport(broadcastManager),
          _messageBus(messageBus), _sensorCommandHandler(sensorCommandHandler),
          _relayController(relayController), _sensorPin(sensorPin), _isDigital(isDigital),
          _name(name), _isDaytime(true), _pendingDaytime(true), _pendingDaytimeCount(DaytimeConfirmReadings),
          _lightQueue(LightQueueCapacity), _currentLightLevel(0)
    {
    }

    void formatStatusJson(char* buffer, size_t size) override
    {
        snprintf(buffer, size, "\"isDaytime\":%s,\"lightLevel\":%u,\"avgLightLevel\":%u",
            _isDaytime ? "true" : "false",
            _currentLightLevel,
            effectiveAverage());
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
        return _name;
    }

    uint16_t getLightLevel() const
    {
        return _currentLightLevel;
    }

    uint16_t getAverageLightLevel() const
    {
        return effectiveAverage();
    }

#if defined(MQTT_SUPPORT)

    uint8_t getMqttChannelCount() const override
    {
        return 3;
    }

    MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
    {
        switch (channelIndex)
        {
            case 1: return { "Light Level", "light_level", nullptr, nullptr, false };
            case 2: return { "Avg Light Level", "avg_light_level", nullptr, nullptr, false };
            default: return { "Daylight", "light", "light", nullptr, true };
        }
    }

    void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
    {
        switch (channelIndex)
        {
            case 1: snprintf(buffer, size, "%u", _currentLightLevel); break;
            case 2: snprintf(buffer, size, "%u", effectiveAverage()); break;
            default: snprintf(buffer, size, "%s", _isDaytime ? "ON" : "OFF"); break;
        }
    }
#endif
};