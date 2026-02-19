#pragma once

#if defined(MQQT_SUPPORT)

#include "MQTTController.h"
#include "MessageBus.h"

// Base class for MQTT domain handlers
class MQTTHandler
{
protected:
    MQTTController* _mqttController;
    MessageBus* _messageBus;
    bool _isSubscribed;

    // Helper for building topic paths
    void buildTopic(char* buffer, size_t bufferSize, const char* subtopic);

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

#endif