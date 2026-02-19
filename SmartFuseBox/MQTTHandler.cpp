#if defined(MQQT_SUPPORT)

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
    end();
}

void MQTTHandler::buildTopic(char* buffer, size_t bufferSize, const char* subtopic)
{
    if (buffer == nullptr || subtopic == nullptr || _mqttController == nullptr)
    {
        return;
    }
    
    Config* config = ConfigManager::getConfigPtr();
    if (config == nullptr)
    {
        return;
    }
    
    snprintf(buffer, bufferSize, "smartfusebox/%s/%s", 
        config->mqtt.deviceId, subtopic);
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

#endif