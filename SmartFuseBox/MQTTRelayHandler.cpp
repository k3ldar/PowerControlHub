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
#include "Local.h"
#include "MQTTRelayHandler.h"
#include "SystemDefinitions.h"
#include <SerialCommandManager.h>
#include <string.h>
#include <stdio.h>

MQTTRelayHandler::MQTTRelayHandler(MQTTController* mqttController, MessageBus* messageBus, RelayController* relayController, SerialCommandManager* commandMgr)
    : MQTTHandler(mqttController, messageBus)
    , _relayController(relayController)
    , _config(nullptr)
    , _commandMgr(commandMgr)
    , _discoveryPending(false)
    , _discoveryIndex(0)
    , _lastDiscoveryPublish(0)
{
    _commandTopic[0] = '\0';
    _config = ConfigManager::getConfigPtr();
}

bool MQTTRelayHandler::begin()
{
    if (_mqttController == nullptr || _messageBus == nullptr || _relayController == nullptr || _config == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Required components not available"), F("MQTT Relay"));
        }
        return false;
    }

    // Subscribe to MessageBus events
    _messageBus->subscribe<RelayStatusChanged>(
        [this](uint8_t status)
        {
            this->onRelayStatusChanged(status);
        }
    );

#if defined(MQTT_SUPPORT)
    // Subscribe to MQTT connected event to re-subscribe and publish states
    _messageBus->subscribe<MqttConnected>(
        [this]()
        {
            this->subscribe();

            // Trigger discovery if enabled (will be processed in update())
            if (_config != nullptr && _config->mqtt.useHomeAssistantDiscovery)
            {
                _discoveryPending = true;
                _discoveryIndex = 0;
            }

            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                if (_config->relay.relays[i].pin == PinDisabled)
                    continue;

                CommandResult statusResult = _relayController->getRelayStatus(i);
                if (statusResult.success)
                {
                    this->publishRelayState(i, statusResult.status == 1);
                }
            }
        }
    );

    // Subscribe to MQTT messages
    _messageBus->subscribe<MqttMessageReceived>(
        [this](const char* topic, const char* payload)
        {
            this->onMessage(topic, payload);
        }
    );
#endif

    return true;
}

void MQTTRelayHandler::update()
{
    unsigned long now = millis();

    if (_discoveryPending && _discoveryIndex < ConfigRelayCount)
    {
        if (_mqttController != nullptr && _mqttController->isConnected())
        {
            // Only publish if enough time has elapsed since last discovery message (200ms minimum)
            if (now - _lastDiscoveryPublish >= MinPublishInterval)
            {
                // Skip disabled relays (no pin assigned)
                if (_config == nullptr || _config->relay.relays[_discoveryIndex].pin != PinDisabled)
                {
                    publishRelayDiscoveryConfig(_discoveryIndex);
                    _lastDiscoveryPublish = now;
                }
                _discoveryIndex++;

                // Check if discovery is complete
                if (_discoveryIndex >= ConfigRelayCount)
                {
                    if (_commandMgr != nullptr)
                    {
                        _commandMgr->sendDebug(F("Home Assistant discovery complete"), F("MQTT"));
                    }
                    _discoveryPending = false;
                }
            }
        }
    }

    }

void MQTTRelayHandler::end()
{
    unsubscribe();
    _isSubscribed = false;
}

void MQTTRelayHandler::onMessage(const char* topic, const char* payload)
{
    if (topic == nullptr || payload == nullptr)
    {
        return;
    }

    // Check if this is a relay command topic (e.g., "home/<device>/relay/3/set")
    uint8_t relayIndex;
    if (extractIndexFromTopic(topic, "relay/", &relayIndex))
    {
        // Verify topic ends with "/set"
        if (strstr(topic, "/set") != nullptr)
        {
            handleRelayCommand(relayIndex, payload);
        }
    }
}

bool MQTTRelayHandler::subscribe()
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Controller or config not available"), F("MQTT Relay"));
        }
        return false;
    }

    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr || !client->isConnected())
    {
        return false;
    }

    // Subscribe to relay command topics: homeassistant/<device>/relay/+/set
    // Subscribe to command topics that the device actually uses for commands
    // Command topics use the "home/<device>/relay/<index>/set" pattern
    char topic[MqttMaxTopicLength];
    snprintf(topic, sizeof(topic), "home/%s/relay/+/set", _config->mqtt.deviceId);

    bool result = client->subscribe(topic, MqttQoS::AtMostOnce);
    if (result)
    {
        _isSubscribed = true;
    }
    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(topic, F("MQTT Relay Subscribe"));
    }

    return result;
}

void MQTTRelayHandler::unsubscribe()
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        return;
    }
    
    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr)
    {
        return;
    }
    
    // Unsubscribe from relay command topics
    char topic[MqttMaxTopicLength];
    snprintf(topic, sizeof(topic), "home/%s/relay/+/set", _config->mqtt.deviceId);
    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(topic, F("MQTT Relay Unsubscribe"));
    }

    client->unsubscribe(topic);
    _isSubscribed = false;
}

// ============================================================================
// Private Methods
// ============================================================================

void MQTTRelayHandler::handleRelayCommand(uint8_t relayIndex, const char* payload)
{
    if (_relayController == nullptr || payload == nullptr)
    {
        return;
    }

    // Debug: log command handling entry
    if (_commandMgr != nullptr)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "relay index: %u", relayIndex);
        _commandMgr->sendDebug(buf, F("MQTT Relay"));
        _commandMgr->sendDebug(payload, F("MQTT Relay payload"));
    }

    // Validate relay index
    if (relayIndex >= ConfigRelayCount)
    {
        if (_commandMgr != nullptr)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Invalid relay index: %u", relayIndex);
            _commandMgr->sendError(buf, F("MQTT Relay"));
        }
        return;
    }

    // Reject commands targeting a disabled relay
    if (_config != nullptr && _config->relay.relays[relayIndex].pin == PinDisabled)
    {
        if (_commandMgr != nullptr)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Relay %u is disabled", relayIndex);
            _commandMgr->sendDebug(buf, F("MQTT Relay"));
        }
        return;
    }
    
    // Parse command (ON, OFF, TOGGLE, 1, 0)
    bool turnOn = false;
    
    if (strcasecmp(payload, "ON") == 0 || strcmp(payload, "1") == 0)
    {
        turnOn = true;
    }
    else if (strcasecmp(payload, "OFF") == 0 || strcmp(payload, "0") == 0)
    {
        turnOn = false;
    }
    else if (strcasecmp(payload, "TOGGLE") == 0)
    {
        // Toggle current state
        CommandResult statusResult = _relayController->getRelayStatus(relayIndex);
        if (statusResult.success)
        {
            bool currentState = (statusResult.status == 1);
            turnOn = !currentState;
        }
        else
        {
            return; // Failed to get current state
        }
    }
    else
    {
        // Unknown command
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(payload, F("MQTT Relay unknown payload"));
        }
        return;
    }

    // Set relay state
    _relayController->setRelayState(relayIndex, turnOn);
    // Log result of setting relay
    CommandResult res = _relayController->getRelayStatus(relayIndex);
    if (_commandMgr != nullptr)
    {
        char buf[40];
        if (res.success)
        {
            snprintf(buf, sizeof(buf), "Relay %u set to %s", relayIndex, res.status == 1 ? "ON" : "OFF");
            _commandMgr->sendDebug(buf, F("MQTT Relay"));
        }
        else
        {
            snprintf(buf, sizeof(buf), "Relay %u error code: %d", relayIndex, res.status);
            _commandMgr->sendError(buf, F("MQTT Relay"));
        }
    }

    // Publish state will be handled by RelayStatusChanged event
}

void MQTTRelayHandler::publishRelayState(uint8_t relayIndex, bool isOn)
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        return;
    }

    if (!_mqttController->isConnected())
    {
        return;
    }

    // Validate relay index
    if (relayIndex >= ConfigRelayCount)
    {
        return;
    }

    // Build state topic: home/<device>/relay<index>/state
    char topic[MqttMaxTopicLength];
    snprintf(topic, sizeof(topic), "home/%s/relay/%u/state", _config->mqtt.deviceId, relayIndex);

	if (_commandMgr != nullptr)
	{
		_commandMgr->sendDebug(topic, F("MQTT Relay State"));
	}

    // Publish state
    const char* payload = isOn ? "ON" : "OFF";
    _mqttController->publishState(topic, payload);
}

void MQTTRelayHandler::onRelayStatusChanged(uint8_t relayBitmask)
{
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        if (relayBitmask & (1 << i))
        {
            if (_config != nullptr && _config->relay.relays[i].pin == PinDisabled)
                continue;

            CommandResult statusResult = _relayController->getRelayStatus(i);
            if (statusResult.success)
            {
                publishRelayState(i, statusResult.status == 1);
            }
        }
    }
}

void MQTTRelayHandler::publishDiscoveryConfig()
{
    // Trigger non-blocking discovery
    _discoveryPending = true;
    _discoveryIndex = 0;
}

void MQTTRelayHandler::publishRelayDiscoveryConfig(uint8_t relayIndex)
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        return;
    }

    if (!_mqttController->isConnected())
    {
        return;
    }

    // Validate relay index
    if (relayIndex >= ConfigRelayCount)
    {
        return;
    }

    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr)
    {
        return;
    }

    // Build discovery topic: <discoveryPrefix>/switch/<device_id>/<object_id>/config
    // Use a single-segment object_id (relay_<index>) so Home Assistant can parse it
    char topic[MqttMaxTopicLength];
    int topicLen = snprintf(topic, sizeof(topic), "%s/switch/%s/relay_%u/config",
        _config->mqtt.discoveryPrefix, _config->mqtt.deviceId, relayIndex);
    if (topicLen < 0 || topicLen >= static_cast<int>(sizeof(topic)))
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Discovery topic truncated"), F("MQTT Relay"));
        }
        return;
    }

    // Build JSON payload
    // Note: Keep this simple to avoid buffer overflow - max ~512 bytes
    char payload[MqttMaxPayloadLength];

    // Get relay name from config (use long name for display)
    const char* relayName = _config->relay.relays[relayIndex].longName;

    // Build minimal JSON discovery payload
    int payloadLen = snprintf(payload, sizeof(payload),
        "{\"name\":\"%s\","
        "\"state_topic\":\"home/%s/relay/%u/state\","
        "\"command_topic\":\"home/%s/relay/%u/set\","
        "\"payload_on\":\"ON\","
        "\"payload_off\":\"OFF\","
        "\"state_on\":\"ON\","
        "\"state_off\":\"OFF\","
        "\"unique_id\":\"%s_relay_%u\","
        "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
        relayName,
        _config->mqtt.deviceId, relayIndex,
        _config->mqtt.deviceId, relayIndex,
        _config->mqtt.deviceId, relayIndex,
        _config->mqtt.deviceId
    );

    if (payloadLen < 0 || payloadLen >= static_cast<int>(sizeof(payload)))
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Discovery payload truncated"), F("MQTT Relay"));
        }
        return;
    }

    // Publish with retain flag
    client->publish(topic, payload, MqttQoS::AtMostOnce, true);
}
