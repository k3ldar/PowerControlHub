#pragma once

#if defined(MQQT_SUPPORT)

#include "MQTTHandler.h"
#include "RelayController.h"
#include "ConfigManager.h"


class MQTTRelayHandler : public MQTTHandler
{
private:
    RelayController* _relayController;
    Config* _config;
    
    // Topic buffers
    char _commandTopic[MqttMaxTopicLength];
    
    // Message handlers
    void handleRelayCommand(uint8_t relayIndex, const char* payload);
    
    // State publishing
    void publishRelayState(uint8_t relayIndex, bool isOn);
    void publishAllRelayStates();
    
    // MessageBus event handler
    void onRelayStatusChanged(uint8_t relayBitmask);

public:
    MQTTRelayHandler(MQTTController* mqttController, MessageBus* messageBus, RelayController* relayController);
    
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

#endif