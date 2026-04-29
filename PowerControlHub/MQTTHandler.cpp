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
#include "Local.h"
#include "MQTTHandler.h"
#include "ConfigManager.h"
#include <string.h>
#include <stdio.h>

MQTTHandler::MQTTHandler(MQTTController* mqttController, MessageBus* messageBus)
    : _mqttController(mqttController)
    , _messageBus(messageBus)
    , _isSubscribed(false)
{
}

MQTTHandler::~MQTTHandler()
{
    // Don't call pure virtual end() from destructor
    // Derived classes must call end() in their own destructors
}

bool MQTTHandler::extractIndexFromTopic(const char* topic, const char* prefix, uint8_t* index)
{
    if (topic == nullptr || prefix == nullptr || index == nullptr)
    {
        return false;
    }
    
    // Find prefix in topic
    const char* pos = strstr(topic, prefix);
    if (pos == nullptr)
    {
        return false;
    }
    
    // Move past prefix
    pos += strlen(prefix);
    
    // Skip any leading slashes
    while (*pos == '/')
    {
        pos++;
    }
    
    // Parse number
    char* endPtr;
    long value = strtol(pos, &endPtr, 10);
    
    if (endPtr == pos || value < 0 || value > 255)
    {
        return false;
    }
    
    *index = (uint8_t)value;
    return true;
}
