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
#include "RelayController.h"
#include "SensorCommandHandler.h"

constexpr uint64_t BinaryPresenceCheckMs = 300;

/**
 * @brief Sensor handler for binary presence monitoring.
 *
 * Reads binary presence sensor values, for a specific GPIO pin,
 * e.g. a PIR motion sensor or a door/window contact sensor
 * and reports readings to both link and computer serial connections.
 */
class BinaryPresenceSensor : public BaseSensor, public BroadcastLoggerSupport
{
private:
	MessageBus* _messageBus;
	SensorCommandHandler* _sensorCommandHandler;
	RelayController* _relayController;
	const uint8_t _sensorPin;
	int _activeState; // HIGH or LOW depending on sensor type
	int _lastState;
	uint64_t _lastChangeMs;

	ExecutionActionType _onDetectedAction;
	uint8_t _onDetectedPayload[ConfigSchedulerPayloadSize];
	ExecutionActionType _onClearAction;
	uint8_t _onClearPayload[ConfigSchedulerPayloadSize];

	// Pulse state for sensor-triggered direct actions (single dedicated slot)
	bool _directPulseActive;
	uint64_t _directPulseStartMs;
	uint64_t _directPulseDurMs;
	uint8_t _directPulseRelayIdx;


#if defined(MQTT_SUPPORT)
	char _slugPresence[32];
	char _namePresence[32];
#endif

	void executeAction(ExecutionActionType actionType, const uint8_t* payload, uint8_t payloadSize = ConfigSchedulerPayloadSize);

protected:
	void initialize() override;
	uint64_t update() override;

public:
	BinaryPresenceSensor(MessageBus* messageBus, BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		RelayController* relayController, uint8_t sensorPin, int activeState, const char* name,
		ExecutionActionType onDetectedAction, uint8_t onDetectedPayload,
		ExecutionActionType onClearAction, uint8_t onClearPayload,
		uint16_t pulseDurationSec = 0);

	void formatStatusJson(char* buffer, size_t size) override;

	SensorIdList getSensorIdType() const override;

	SensorType getSensorType() const override;

	const char* getSensorCommandId() const override;

#if defined(MQTT_SUPPORT)
	uint8_t getMqttChannelCount() const override;

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override;

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override;
#endif
};