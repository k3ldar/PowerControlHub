
#if defined(MQQT_SUPPORT)

#include "MQTTRelayHandler.h"
#include "SystemDefinitions.h"
#include <string.h>
#include <stdio.h>

MQTTRelayHandler::MQTTRelayHandler(MQTTController* mqttController, MessageBus* messageBus, RelayController* relayController)
    : MQTTHandler(mqttController, messageBus)
    , _relayController(relayController)
    , _config(nullptr)
{
    _commandTopic[0] = '\0';
    _config = ConfigManager::getConfigPtr();
}

bool MQTTRelayHandler::begin()
{
    if (_mqttController == nullptr || _messageBus == nullptr || _relayController == nullptr || _config == nullptr)
    {
        return false;
    }
    
    // Subscribe to MessageBus events
    _messageBus->subscribe<RelayStatusChanged>(
        [this](uint8_t status)
        {
            this->onRelayStatusChanged(status);
        }
    );
    
    // Subscribe to MQTT connected event to re-subscribe and publish states
    _messageBus->subscribe<MqttConnected>(
        [this]()
        {
            this->subscribe();
            this->publishAllRelayStates();
        }
    );
    
    // Subscribe to MQTT messages
    _messageBus->subscribe<MqttMessageReceived>(
        [this](const char* topic, const char* payload)
        {
            this->onMessage(topic, payload);
        }
    );
    
    return true;
}

void MQTTRelayHandler::update()
{
    // Nothing to do in update for relay handler
    // All actions are event-driven via MessageBus
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
    
    // Check if this is a relay command topic (e.g., "smartfusebox/<device>/relay/3/set")
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
        return false;
    }
    
    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr || !client->isConnected())
    {
        return false;
    }
    
    // Subscribe to relay command topics: smartfusebox/<device>/relay/+/set
    char topic[MqttMaxTopicLength];
    snprintf(topic, sizeof(topic), "smartfusebox/%s/relay/+/set", _config->mqtt.deviceId);
    
    bool result = client->subscribe(topic, MqttQoS::AtMostOnce);
    if (result)
    {
        _isSubscribed = true;
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
    snprintf(topic, sizeof(topic), "smartfusebox/%s/relay/+/set", _config->mqtt.deviceId);
    
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
    
    // Validate relay index
    if (relayIndex >= ConfigRelayCount)
    {
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
        turnOn = !_relayController->getRelayStatus(relayIndex);
    }
    else
    {
        // Unknown command
        return;
    }
    
    // Set relay state
    RelayResult result = _relayController->setRelay(relayIndex, turnOn);
    
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
    
    // Build state topic: smartfusebox/<device>/relay/<index>/state
    char topic[MqttMaxTopicLength];
    snprintf(topic, sizeof(topic), "relay/%u/state", relayIndex);
    
    // Publish state
    const char* payload = isOn ? "ON" : "OFF";
    _mqttController->publishState(topic, payload);
}

void MQTTRelayHandler::publishAllRelayStates()
{
    if (_relayController == nullptr)
    {
        return;
    }
    
    // Publish state for each relay
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        bool isOn = _relayController->getRelayStatus(i);
        publishRelayState(i, isOn);
    }
}

void MQTTRelayHandler::onRelayStatusChanged(uint8_t relayBitmask)
{
    // Publish state for each relay
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        bool isOn = (relayBitmask & (1 << i)) != 0;
        publishRelayState(i, isOn);
    }
}

#endif