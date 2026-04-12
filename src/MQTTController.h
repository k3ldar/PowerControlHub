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
#pragma once

#include "MQTTClient.h"
#include "MessageBus.h"
#include "IWifiRadio.h"
#include "SystemFunctions.h"

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
    uint64_t _lastRetryTime;
    uint64_t _lastDebugTime;
    bool _isEnabled;
    
    SerialCommandManager* _commandMgr;

    // Statistics
    uint64_t _connectedSince;
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
    uint64_t getUptime() const;
    
    // Access to client for advanced usage
    MQTTClient* getClient();
};
