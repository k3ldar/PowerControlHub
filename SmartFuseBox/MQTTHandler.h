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

#include "MQTTController.h"
#include "MessageBus.h"

// Base class for MQTT domain handlers
class MQTTHandler
{
protected:
    MQTTController* _mqttController;
    MessageBus* _messageBus;
    bool _isSubscribed;

    // Helper for extracting index from topic (e.g., "relay/3/set" -> 3)
    bool extractIndexFromTopic(const char* topic, const char* prefix, uint8_t* index);

public:
    MQTTHandler(MQTTController* mqttController, MessageBus* messageBus);
    virtual ~MQTTHandler();

    // Lifecycle - to be implemented by derived classes
    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual void end() = 0;

    // Message handling - to be implemented by derived classes
    virtual void onMessage(const char* topic, const char* payload) = 0;

    // Subscription management
    virtual bool subscribe() = 0;
    virtual void unsubscribe() = 0;
};
