#pragma once

#include "MQTTHandler.h"
#include "RelayController.h"
#include "ConfigManager.h"

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
    unsigned long _lastDiscoveryPublish;

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
