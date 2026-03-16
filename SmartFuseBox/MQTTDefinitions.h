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

#include <stdint.h>

// MQTT Protocol Version
constexpr uint8_t MqttProtocolVersion = 0x04; // MQTT 3.1.1

// MQTT Control Packet Types
enum class MqttPacketType : uint8_t
{
    Reserved0 = 0x00,
    Connect = 0x10,
    ConnAck = 0x20,
    Publish = 0x30,
    PubAck = 0x40,
    PubRec = 0x50,
    PubRel = 0x60,
    PubComp = 0x70,
    Subscribe = 0x80,
    SubAck = 0x90,
    Unsubscribe = 0xA0,
    UnsubAck = 0xB0,
    PingReq = 0xC0,
    PingResp = 0xD0,
    Disconnect = 0xE0,
    Reserved15 = 0xF0
};

// MQTT QoS Levels
enum class MqttQoS : uint8_t
{
    AtMostOnce = 0x00,      // Fire and forget
    AtLeastOnce = 0x01,     // Acknowledged delivery
    ExactlyOnce = 0x02      // Assured delivery (not implemented)
};

// MQTT Connection States
enum class MqttConnectionState : uint8_t
{
    Disconnected = 0x00,
    Connecting = 0x01,
    Connected = 0x02,
    Disconnecting = 0x03,
    Error = 0x04
};

// MQTT Connect Return Codes
enum class MqttConnectReturnCode : uint8_t
{
    Accepted = 0x00,
    UnacceptableProtocolVersion = 0x01,
    IdentifierRejected = 0x02,
    ServerUnavailable = 0x03,
    BadUserNameOrPassword = 0x04,
    NotAuthorized = 0x05
};

// MQTT Error Codes
enum class MqttError : uint8_t
{
    None = 0x00,
    BufferOverflow = 0x01,
    InvalidPacket = 0x02,
    NetworkError = 0x03,
    ProtocolError = 0x04,
    Timeout = 0x05,
    NotConnected = 0x06,
    AlreadyConnected = 0x07,
    InvalidParameter = 0x08
};

// MQTT Buffer Sizes
constexpr uint16_t MqttMaxPacketSize = 512;
constexpr uint8_t MqttMaxTopicLength = 128;
constexpr uint16_t MqttMaxPayloadLength = 512;
constexpr uint8_t MqttClientIdLength = 32;

// Sentinel value used to indicate the remaining-length field has not yet been decoded.
// Must never equal a valid decoded remaining length (max supported payload is 512 so 0xFFFF is safe).
constexpr uint16_t MqttRxLengthNotDecoded = 0xFFFF;

// MQTT Timeouts (milliseconds)
constexpr uint16_t MqttConnectTimeout = 5000;
constexpr uint16_t MqttKeepAliveDefault = 60000;
constexpr uint16_t MqttPingTimeout = 3000;

// MQTT Retry Configuration
constexpr uint8_t MqttMaxRetries = 5;
constexpr uint16_t MqttRetryDelays[] = {0, 5000, 15000, 60000, 60000};

// MQTT Connect Flags
constexpr uint8_t MqttConnFlagCleanSession = 0x02;
constexpr uint8_t MqttConnFlagWill = 0x04;
constexpr uint8_t MqttConnFlagWillQoS1 = 0x08;
constexpr uint8_t MqttConnFlagWillQoS2 = 0x10;
constexpr uint8_t MqttConnFlagWillRetain = 0x20;
constexpr uint8_t MqttConnFlagPassword = 0x40;
constexpr uint8_t MqttConnFlagUsername = 0x80;

// MQTT Publish Flags
constexpr uint8_t MqttPublishFlagRetain = 0x01;
constexpr uint8_t MqttPublishFlagQoS1 = 0x02;
constexpr uint8_t MqttPublishFlagQoS2 = 0x04;
constexpr uint8_t MqttPublishFlagDup = 0x08;

// MQTT Subscribe/Unsubscribe Flags
constexpr uint8_t MqttSubscribeFlags = 0x02;  // QoS 1 for SUBSCRIBE
constexpr uint8_t MqttUnsubscribeFlags = 0x02;  // QoS 1 for UNSUBSCRIBE

// MQTT Remaining Length Encoding
constexpr uint8_t MqttLengthContinuationBit = 0x80;
constexpr uint8_t MqttLengthValueMask = 0x7F;

// MQTT Packet Identifier
constexpr uint16_t MqttPacketIdMin = 1;
constexpr uint16_t MqttPacketIdMax = 65535;

// MQTT Callback Types
typedef void (*MqttMessageCallback)(const char* topic, const char* payload, uint16_t length);
typedef void (*MqttConnectionCallback)(bool connected);

// MQTT Event Types for diagnostics/logging
enum class MqttEvent : uint8_t
{
    None = 0x00,

    // Connection events
    Connected = 0x01,
    Disconnected = 0x02,
    ConnectionRefused = 0x03,
    ConnectionTimeout = 0x04,

    // Error events
    BrokerNotConfigured = 0x10,
    BufferOverflow = 0x11,
    InvalidPacket = 0x12,
    NetworkError = 0x13,
    KeepAliveTimeout = 0x14,

    // Packet events (optional, for verbose logging)
    PacketSent = 0x20,
    PacketReceived = 0x21
};

// Event callback with optional error code parameter
typedef void (*MqttEventCallback)(MqttEvent event, uint8_t errorCode);
