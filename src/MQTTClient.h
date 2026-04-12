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

#include <Arduino.h>
#include "IWifiClient.h"
#include "MQTTDefinitions.h"
#include "SystemFunctions.h"

// Forward declarations
class SerialCommandManager;
class IWifiRadio;

class MQTTClient
{
private:
    IWifiClient* _wifiClient;
    IWifiRadio* _wifiRadio;
    MqttConnectionState _state;
    MqttError _lastError;
    
    // Connection parameters
    char _broker[64];
    uint16_t _port;
    char _clientId[MqttClientIdLength];
    char _username[32];
    char _password[32];
    bool _useCredentials;
    uint16_t _keepAlive;
    
    // Packet buffers
    uint8_t _rxBuffer[MqttMaxPacketSize];
    uint8_t _txBuffer[MqttMaxPacketSize];
    uint16_t _rxBufferPos;
    uint16_t _rxPacketLength;
    
    // Topic and payload buffers
    char _topicBuffer[MqttMaxTopicLength];
    char _payloadBuffer[MqttMaxPayloadLength];
    
    // Packet identifier counter
    uint16_t _packetId;
    
    // Timing
    uint64_t _lastSendTime;
    uint64_t _lastReceiveTime;
    uint64_t _connectAttemptTime;
    
    // Callbacks
    MqttMessageCallback _messageCallback;
    MqttConnectionCallback _connectionCallback;
    MqttEventCallback _eventCallback;
    
    // Statistics
    uint32_t _packetsSent;
    uint32_t _packetsReceived;

    // Debug output
    SerialCommandManager* _commandMgr;
    
    // Internal helper methods
    uint16_t getNextPacketId();
    uint8_t encodeRemainingLength(uint8_t* buffer, uint32_t length);
    uint32_t decodeRemainingLength(const uint8_t* buffer, uint8_t* bytes);
    bool writePacket(const uint8_t* buffer, uint16_t length);
    bool readPacket();
    bool processPacket(uint8_t packetType, const uint8_t* payload, uint16_t length);
    
    // Packet builders
    uint16_t buildConnectPacket(uint8_t* buffer, uint16_t bufferSize);
    uint16_t buildPublishPacket(uint8_t* buffer, uint16_t bufferSize,
        const char* topic, const char* payload, uint16_t payloadLen,
        MqttQoS qos, bool retain);
    uint16_t buildSubscribePacket(uint8_t* buffer, uint16_t bufferSize,
        const char* topic, MqttQoS qos);
    uint16_t buildUnsubscribePacket(uint8_t* buffer, uint16_t bufferSize,
        const char* topic);
    uint16_t buildPingReqPacket(uint8_t* buffer, uint16_t bufferSize);
    uint16_t buildDisconnectPacket(uint8_t* buffer, uint16_t bufferSize);
    
    // Packet processors
    bool processConnAck(const uint8_t* payload, uint16_t length);
    bool processPublish(const uint8_t* payload, uint16_t length);
    bool processPingResp(const uint8_t* payload, uint16_t length);
    
    // Utility
    void resetBuffers();
    void setError(MqttError error);
    void raiseEvent(MqttEvent event, uint8_t errorCode = 0);

public:
    explicit MQTTClient(IWifiRadio* wifiRadio = nullptr);
    ~MQTTClient();

    // Configuration
    void setBroker(const char* broker, uint16_t port);
    void setCredentials(const char* username, const char* password);
    void setClientId(const char* clientId);
    void setKeepAlive(uint16_t seconds);
    void setCommandManager(SerialCommandManager* commandMgr);
    void setMessageCallback(MqttMessageCallback callback);
    void setConnectionCallback(MqttConnectionCallback callback);
    void setEventCallback(MqttEventCallback callback);
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    MqttConnectionState getState() const;
    MqttError getLastError() const;
    
    bool update();
    
    // Publishing
    bool publish(const char* topic, const char* payload, MqttQoS qos, bool retain);
    bool publish(const char* topic, const uint8_t* payload, uint16_t length, MqttQoS qos, bool retain);
    
    // Subscription
    bool subscribe(const char* topic, MqttQoS qos);
    bool unsubscribe(const char* topic);
    
    // Statistics
    uint32_t getPacketsSent() const;
    uint32_t getPacketsReceived() const;
    
    // Diagnostics
    void resetStatistics();
};

