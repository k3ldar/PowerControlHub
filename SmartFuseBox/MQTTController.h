#pragma once

#include "MQTTClient.h"
#include "MessageBus.h"
#include "IWifiRadio.h"

// Forward declarations
struct Config;
class SerialCommandManager;

class MQTTController
{
private:
    MQTTClient* _mqttClient;
    MessageBus* _messageBus;
    Config* _config;
    IWifiRadio* _wifiRadio;
    
    // Connection management
    uint8_t _retryCount;
    unsigned long _lastRetryTime;
    bool _isEnabled;
    
    SerialCommandManager* _commandMgr;

    // Statistics
    unsigned long _connectedSince;
    uint32_t _reconnectCount;
    
    // Internal helpers
    void onMqttConnected(bool connected);
    void onMqttMessage(const char* topic, const char* payload, uint16_t length);
    void onMqttEvent(MqttEvent event, uint8_t errorCode);
    bool shouldRetry();
    uint16_t getRetryDelay();

    // Static callbacks (bridge to instance methods)
    static void connectionCallbackStatic(bool connected);
    static void messageCallbackStatic(const char* topic, const char* payload, uint16_t length);
    static void eventCallbackStatic(MqttEvent event, uint8_t errorCode);
    static MQTTController* _instance;

public:
    MQTTController(MessageBus* messageBus, Config* config, IWifiRadio* wifiRadio, SerialCommandManager* commandMgr = nullptr);
    ~MQTTController();
    
    // Lifecycle
    bool begin();
    void update();
    void end();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    
    // Publishing helpers
    bool publishState(const char* topic, const char* value);
    
    // Statistics
    uint32_t getReconnectCount() const;
    unsigned long getUptime() const;
    
    // Access to client for advanced usage
    MQTTClient* getClient();
};
