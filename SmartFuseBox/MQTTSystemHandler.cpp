#include "Local.h"

#if defined(MQTT_SUPPORT)
#include "MQTTSystemHandler.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include <SerialCommandManager.h>
#include <string.h>
#include <stdio.h>

MQTTSystemHandler::MQTTSystemHandler(MQTTController* mqttController, MessageBus* messageBus, SerialCommandManager* commandMgr)
    : MQTTHandler(mqttController, messageBus)
    , _commandMgr(commandMgr)
    , _config(nullptr)
{
    _timeTopic[0] = '\0';
    _config = ConfigManager::getConfigPtr();
}

bool MQTTSystemHandler::begin()
{
    if (_mqttController == nullptr || _messageBus == nullptr || _config == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Required components not available"), F("MQTT System"));
        }
        return false;
    }

    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(F("MQTTSystemHandler initializing"), F("MQTT System"));
    }

    // Subscribe to MQTT connected event to re-subscribe
    _messageBus->subscribe<MqttConnected>(
        [this]()
        {
            if (this->_commandMgr != nullptr)
            {
                this->_commandMgr->sendDebug(F("MQTT Connected event received"), F("MQTT System"));
            }
            this->subscribe();
        }
    );

    // Subscribe to MQTT messages
    _messageBus->subscribe<MqttMessageReceived>(
        [this](const char* topic, const char* payload)
        {
            this->onMessage(topic, payload);
        }
    );

    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(F("MQTTSystemHandler initialized successfully"), F("MQTT System"));
    }

    return true;
}

void MQTTSystemHandler::update()
{
    // No periodic tasks for now
}

void MQTTSystemHandler::end()
{
    unsubscribe();
    _isSubscribed = false;
}

void MQTTSystemHandler::onMessage(const char* topic, const char* payload)
{
    if (topic == nullptr || payload == nullptr)
    {
        return;
    }

    // Check if this is a time update topic
    if (strstr(topic, "/time") != nullptr)
    {
        handleTimeUpdate(payload);
    }
}

bool MQTTSystemHandler::subscribe()
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Controller or config not available"), F("MQTT System"));
        }
        return false;
    }

    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr || !client->isConnected())
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Client not available or not connected"), F("MQTT System"));
        }
        return false;
    }

    // Build time topic: home/<deviceId>/time
    // Note: Uses "home" prefix to match sensor topics, not discoveryPrefix
    snprintf(_timeTopic, sizeof(_timeTopic), "home/%s/time",
        _config->mqtt.deviceId);

    if (_commandMgr != nullptr)
    {
        char buf[150];
        snprintf(buf, sizeof(buf), "Attempting to subscribe to: %s", _timeTopic);
        _commandMgr->sendDebug(buf, F("MQTT System"));
    }

    bool success = client->subscribe(_timeTopic, MqttQoS::AtMostOnce);

    if (success)
    {
        _isSubscribed = true;
        if (_commandMgr != nullptr)
        {
            char buf[150];
            snprintf(buf, sizeof(buf), "Successfully subscribed to: %s", _timeTopic);
            _commandMgr->sendDebug(buf, F("MQTT System"));
        }
    }
    else
    {
        if (_commandMgr != nullptr)
        {
            char buf[150];
            snprintf(buf, sizeof(buf), "Failed to subscribe to topic: %s", _timeTopic);
            _commandMgr->sendError(buf, F("MQTT System"));
        }
    }

    return success;
}

void MQTTSystemHandler::unsubscribe()
{
    if (!_isSubscribed || _mqttController == nullptr)
    {
        return;
    }

    MQTTClient* client = _mqttController->getClient();
    if (client != nullptr && client->isConnected() && _timeTopic[0] != '\0')
    {
        client->unsubscribe(_timeTopic);
    }

    _isSubscribed = false;
}

void MQTTSystemHandler::handleTimeUpdate(const char* payload)
{
    if (payload == nullptr || payload[0] == '\0')
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Empty time payload"), F("MQTT System"));
        }
        return;
    }

    // Parse Unix timestamp from payload
    char* endPtr;
    unsigned long timestamp = strtoul(payload, &endPtr, 10);

    // Validate the timestamp
    if (endPtr == payload || timestamp == 0)
    {
        if (_commandMgr != nullptr)
        {
            char buf[100];
            snprintf(buf, sizeof(buf), "Invalid timestamp: %s", payload);
            _commandMgr->sendError(buf, F("MQTT System"));
        }
        return;
    }

    // Update DateTimeManager
    DateTimeManager::setDateTime(timestamp);

    if (_commandMgr != nullptr)
    {
        char buf[100];
        snprintf(buf, sizeof(buf), "Time synced: %lu", timestamp);
        _commandMgr->sendDebug(buf, F("MQTT System"));
    }
}
#endif