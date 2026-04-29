/*
 * PowerControlHub
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

#include "MQTTHandler.h"
#include "RelayController.h"
#include "ConfigManager.h"
#include "SystemFunctions.h"

// Forward declaration
class SerialCommandManager;

class MQTTRelayHandler : public MQTTHandler
{
private:
    RelayController* _relayController;
    Config* _config;
    SerialCommandManager* _commandMgr;

    // Topic buffers
    char _commandTopic[MqttMaxTopicLength];

    // Discovery state
    bool _discoveryPending;
    uint8_t _discoveryIndex;
    uint64_t _lastDiscoveryPublish;

    static constexpr uint16_t MinPublishInterval = 200;

    // Message handlers
    void handleRelayCommand(uint8_t relayIndex, const char* payload);

    // State publishing
    void publishRelayState(uint8_t relayIndex, bool isOn);

    // Home Assistant Discovery
    void publishDiscoveryConfig();
    void publishRelayDiscoveryConfig(uint8_t relayIndex);

    // MessageBus event handler
    void onRelayStatusChanged(uint8_t relayBitmask);

public:
    MQTTRelayHandler(MQTTController* mqttController, MessageBus* messageBus, RelayController* relayController, SerialCommandManager* commandMgr);

    // Lifecycle (inherited from MQTTHandler)
    bool begin() override;
    void update() override;
    void end() override;
    
    // Message handling (inherited from MQTTHandler)
    void onMessage(const char* topic, const char* payload) override;
    
    // Subscription management (inherited from MQTTHandler)
    bool subscribe() override;
    void unsubscribe() override;
};
