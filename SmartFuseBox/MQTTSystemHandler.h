#pragma once

#include "MQTTHandler.h"

// Forward declarations
class SerialCommandManager;

class MQTTSystemHandler : public MQTTHandler
{
private:
    SerialCommandManager* _commandMgr;
    char _timeTopic[MqttMaxTopicLength];
    struct Config* _config;

    void handleTimeUpdate(const char* payload);

public:
    MQTTSystemHandler(MQTTController* mqttController, MessageBus* messageBus, SerialCommandManager* commandMgr);

    bool begin() override;
    void update() override;
    void end() override;

    void onMessage(const char* topic, const char* payload) override;

    bool subscribe() override;
    void unsubscribe() override;
};
