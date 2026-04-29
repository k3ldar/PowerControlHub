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


#include "BinaryPresenceSensor.h"
#include "SystemFunctions.h"

BinaryPresenceSensor::BinaryPresenceSensor(MessageBus* messageBus, BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
    RelayController* relayController, uint8_t sensorPin, int activeState, const char* name,
    ExecutionActionType onDetectedAction, uint8_t onDetectedPayload,
    ExecutionActionType onClearAction, uint8_t onClearPayload,
    uint16_t pulseDurationSec)
    : BaseSensor(name), BroadcastLoggerSupport(broadcastManager), _messageBus(messageBus), _sensorCommandHandler(sensorCommandHandler),
    _relayController(relayController), _sensorPin(sensorPin), _activeState(activeState), _lastState(-1), _lastChangeMs(0),
    _onDetectedAction(onDetectedAction), _onClearAction(onClearAction),
    _directPulseActive(false),
    _directPulseStartMs(0),
    _directPulseDurMs(0),
    _directPulseRelayIdx(0)
{
    memset(_onDetectedPayload, 0, sizeof(_onDetectedPayload));
    _onDetectedPayload[0] = onDetectedPayload;
    _onDetectedPayload[1] = static_cast<uint8_t>(pulseDurationSec & 0xFF);
    _onDetectedPayload[2] = static_cast<uint8_t>((pulseDurationSec >> 8) & 0xFF);
    memset(_onClearPayload, 0, sizeof(_onClearPayload));
    _onClearPayload[0] = onClearPayload;
    _onClearPayload[1] = static_cast<uint8_t>(pulseDurationSec & 0xFF);
    _onClearPayload[2] = static_cast<uint8_t>((pulseDurationSec >> 8) & 0xFF);

#if defined(MQTT_SUPPORT)
    snprintf(_slugPresence, sizeof(_slugPresence), "%s_presence", _safeSlug);
    snprintf(_namePresence, sizeof(_namePresence), "%s Presence", _name);
#endif
}

void BinaryPresenceSensor::formatStatusJson(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return;

    const char* stateStr = (_lastState == _activeState) ? "detected" : "clear";
    int written = snprintf(buffer, size, "\"name\":\"%s\",\"SensorPin\":%u,\"state\":\"%s\"",
        _name, _sensorPin, stateStr);

    if (written < 0 || (size_t)written >= size)
    {
        sendError("Status JSON truncated", _name);
        buffer[size - 1] = '\0';
    }
}

SensorIdList BinaryPresenceSensor::getSensorIdType() const
{
    return SensorIdList::BinaryPresenceSensor;
}

SensorType BinaryPresenceSensor::getSensorType() const
{
    return SensorType::Local;
}

const char* BinaryPresenceSensor::getSensorCommandId() const
{
    return SensorBinaryPresence;
}

#if defined(MQTT_SUPPORT)
uint8_t BinaryPresenceSensor::getMqttChannelCount() const
{
    return 1;
}

MqttSensorChannel BinaryPresenceSensor::getMqttChannel(uint8_t channelIndex) const
{
    (void)channelIndex;
    return { _namePresence, _slugPresence, "binary_sensor", "motion", nullptr, true };
}

void BinaryPresenceSensor::getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const
{
    if (!buffer || size == 0)
        return;

    const char* valueStr = (_lastState == _activeState) ? "true" : "false";
	snprintf(buffer, size, "%s", valueStr);
}
#endif


void BinaryPresenceSensor::initialize()
{
    pinMode(_sensorPin, INPUT);
	_lastState = digitalRead(_sensorPin);
}

uint64_t BinaryPresenceSensor::update()
{
    if (_directPulseActive && _relayController)
    {
        if (SystemFunctions::hasElapsed(_directPulseStartMs, _directPulseDurMs))
        {
            _relayController->setRelayState(_directPulseRelayIdx, false);
            _directPulseActive = false;
        }
    }

    int state = digitalRead(_sensorPin);

    if (state != _lastState)
    {
        _lastState = state;
        _lastChangeMs = SystemFunctions::millis64();
        bool detected = (state == _activeState);

        StringKeyValue params[2];
        strncpy(params[0].key, ValueParamName, sizeof(params[0].key));
        snprintf(params[0].value, sizeof(params[0].value), "%d", detected ? 1 : 0);
        strncpy(params[1].key, "name", sizeof(params[1].key));
        strncpy(params[1].value, _name, sizeof(params[1].value));
        sendCommand(SensorBinaryPresence, params, 2);

        if (_messageBus)
            _messageBus->publish<BinaryPresenceUpdated>(detected, _name);

        if (detected)
            executeAction(_onDetectedAction, _onDetectedPayload, ConfigSchedulerPayloadSize);
        else
            executeAction(_onClearAction, _onClearPayload, ConfigSchedulerPayloadSize);
    }

    return BinaryPresenceCheckMs;
}

void BinaryPresenceSensor::executeAction(ExecutionActionType actionType, const uint8_t* payload, uint8_t payloadSize)
{
    switch (actionType)
    {
    case ExecutionActionType::RelayOn:
        _relayController->setRelayState(payload[0], true);
        break;

    case ExecutionActionType::RelayOff:
        _relayController->setRelayState(payload[0], false);
        break;

    case ExecutionActionType::RelayToggle:
    {
        CommandResult current = _relayController->getRelayStatus(payload[0]);
        _relayController->setRelayState(payload[0], current.status == 0);
        break;
    }

    case ExecutionActionType::RelayPulse:
    {
        if (payloadSize < 3)
            break;

        uint16_t durationSec = static_cast<uint16_t>(payload[1] | (payload[2] << 8));
        if (durationSec == 0)
            break;

        uint8_t relayIdx = payload[0];
        _relayController->setRelayState(relayIdx, true);
        _directPulseStartMs = SystemFunctions::millis64();
        _directPulseDurMs = static_cast<uint64_t>(durationSec) * MillisecondsPerSecond;
        _directPulseRelayIdx = relayIdx;
        _directPulseActive = true;
        break;
    }

    case ExecutionActionType::AllRelaysOn:
        _relayController->turnAllRelaysOn();
        break;

    case ExecutionActionType::AllRelaysOff:
        _relayController->turnAllRelaysOff();
        break;

    case ExecutionActionType::SetPinHigh:
    {
        uint8_t pin = payload[0];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
        break;
    }

    case ExecutionActionType::SetPinLow:
    {
        uint8_t pin = payload[0];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        break;
    }

    case ExecutionActionType::None:
    default:
        break;
    }
}
