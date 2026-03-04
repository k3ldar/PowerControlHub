#include "MQTTClient.h"
#include "IWifiRadio.h"
#include <SerialCommandManager.h>
#include <string.h>

MQTTClient::MQTTClient(IWifiRadio* wifiRadio)
    : _wifiClient(nullptr)
    , _wifiRadio(wifiRadio)
    , _state(MqttConnectionState::Disconnected)
    , _lastError(MqttError::None)
    , _port(1883)
    , _useCredentials(false)
    , _keepAlive(60)
    , _rxBufferPos(0)
    , _rxPacketLength(MqttRxLengthNotDecoded)
    , _packetId(MqttPacketIdMin)
    , _lastSendTime(0)
    , _lastReceiveTime(0)
    , _connectAttemptTime(0)
    , _messageCallback(nullptr)
    , _connectionCallback(nullptr)
    , _eventCallback(nullptr)
    , _packetsSent(0)
    , _packetsReceived(0)
    , _commandMgr(nullptr)
{
    _broker[0] = '\0';
    _clientId[0] = '\0';
    _username[0] = '\0';
    _password[0] = '\0';
    resetBuffers();
}

MQTTClient::~MQTTClient()
{
    if (_wifiClient != nullptr)
    {
        _wifiClient->stop();
        delete _wifiClient;
        _wifiClient = nullptr;
    }
}

void MQTTClient::setBroker(const char* broker, uint16_t port)
{
    if (broker == nullptr)
    {
        return;
    }
    
    strncpy(_broker, broker, sizeof(_broker) - 1);
    _broker[sizeof(_broker) - 1] = '\0';
    _port = port;
}

void MQTTClient::setCredentials(const char* username, const char* password)
{
    if (username == nullptr || password == nullptr)
    {
        _useCredentials = false;
        return;
    }
    
    strncpy(_username, username, sizeof(_username) - 1);
    _username[sizeof(_username) - 1] = '\0';
    
    strncpy(_password, password, sizeof(_password) - 1);
    _password[sizeof(_password) - 1] = '\0';
    
    _useCredentials = true;
}

void MQTTClient::setClientId(const char* clientId)
{
    if (clientId == nullptr)
    {
        return;
    }
    
    strncpy(_clientId, clientId, sizeof(_clientId) - 1);
    _clientId[sizeof(_clientId) - 1] = '\0';
}

void MQTTClient::setKeepAlive(uint16_t seconds)
{
    _keepAlive = seconds;
}

void MQTTClient::setMessageCallback(MqttMessageCallback callback)
{
    _messageCallback = callback;
}

void MQTTClient::setConnectionCallback(MqttConnectionCallback callback)
{
    _connectionCallback = callback;
}

void MQTTClient::setEventCallback(MqttEventCallback callback)
{
    _eventCallback = callback;
}

void MQTTClient::setCommandManager(SerialCommandManager* commandMgr)
{
    _commandMgr = commandMgr;
}

bool MQTTClient::connect()
{
    if (_state == MqttConnectionState::Connected)
    {
        setError(MqttError::AlreadyConnected);
        return false;
    }

    if (_broker[0] == '\0')
    {
        raiseEvent(MqttEvent::BrokerNotConfigured);
        setError(MqttError::InvalidParameter);
        return false;
    }

    // Create WiFiClient if needed
    if (_wifiClient == nullptr)
    {
        if (_wifiRadio == nullptr)
        {
            setError(MqttError::InvalidParameter);
            return false;
        }
        _wifiClient = _wifiRadio->createClient();
        if (_wifiClient == nullptr)
        {
            setError(MqttError::NetworkError);
            return false;
        }
    }

    // Attempt TCP connection
    if (!_wifiClient->connect(_broker, _port))
    {
        setError(MqttError::NetworkError);
        return false;
    }
    
    _state = MqttConnectionState::Connecting;
    _connectAttemptTime = millis();

    // Build and send CONNECT packet
    uint16_t packetLength = buildConnectPacket(_txBuffer, sizeof(_txBuffer));
    if (packetLength == 0)
    {
        raiseEvent(MqttEvent::BufferOverflow);
        setError(MqttError::BufferOverflow);
        _wifiClient->stop();
        _state = MqttConnectionState::Disconnected;
        return false;
    }

    if (!writePacket(_txBuffer, packetLength))
    {
        raiseEvent(MqttEvent::NetworkError);
        setError(MqttError::NetworkError);
        _wifiClient->stop();
        _state = MqttConnectionState::Disconnected;
        return false;
    }

    return true;
}

void MQTTClient::disconnect()
{
    if (_state == MqttConnectionState::Disconnected)
    {
        return;
    }

    _state = MqttConnectionState::Disconnecting;
    
    // Send DISCONNECT packet
    uint16_t packetLength = buildDisconnectPacket(_txBuffer, sizeof(_txBuffer));
    if (packetLength > 0)
    {
        writePacket(_txBuffer, packetLength);
    }
    
    if (_wifiClient != nullptr)
    {
        _wifiClient->stop();
    }
    
    _state = MqttConnectionState::Disconnected;

    if (_connectionCallback != nullptr)
    {
        _connectionCallback(false);
    }
}

bool MQTTClient::isConnected() const
{
    return _state == MqttConnectionState::Connected;
}

MqttConnectionState MQTTClient::getState() const
{
    return _state;
}

MqttError MQTTClient::getLastError() const
{
    return _lastError;
}

bool MQTTClient::update()
{
    if (_wifiClient == nullptr)
    {
        return false;
    }

    // Check TCP connection
    if (!_wifiClient->connected() && _state != MqttConnectionState::Disconnected)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("TCP connection lost unexpectedly"), F("MQTT Client"));
        }
        disconnect();
        return false;
    }

    // Handle connecting state timeout
    if (_state == MqttConnectionState::Connecting)
    {
        unsigned long waitMs = millis() - _connectAttemptTime;
        if (waitMs > MqttConnectTimeout)
        {
            if (_commandMgr != nullptr)
            {
                char buf[48];
                snprintf(buf, sizeof(buf), "Connection timeout: waited %lums", waitMs);
                _commandMgr->sendError(buf, F("MQTT Client"));
            }
            raiseEvent(MqttEvent::ConnectionTimeout);
            setError(MqttError::Timeout);
            disconnect();
            return false;
        }
    }

    // Process incoming packets (non-blocking, one packet per update)
    if (_wifiClient->available() > 0)
    {
        if (!readPacket())
        {
            // Not necessarily an error - may be a partial packet waiting for more data
            return false;
        }
    }

    // Keep-alive management
    if (_state == MqttConnectionState::Connected)
    {
        unsigned long now = millis();
        unsigned long timeSinceLastSend = now - _lastSendTime;
        unsigned long timeSinceLastReceive = now - _lastReceiveTime;
        unsigned long keepAliveMs = _keepAlive * 1000UL;

        // Send PINGREQ if no packet sent within keep-alive interval
        if (timeSinceLastSend > keepAliveMs)
        {
            if (_commandMgr != nullptr)
            {
                char buf[48];
                snprintf(buf, sizeof(buf), "Sending PINGREQ (no send for %lums)", timeSinceLastSend);
                _commandMgr->sendDebug(buf, F("MQTT Client"));
            }
            uint16_t packetLength = buildPingReqPacket(_txBuffer, sizeof(_txBuffer));
            if (packetLength > 0)
            {
                if (!writePacket(_txBuffer, packetLength))
                {
                    if (_commandMgr != nullptr)
                    {
                        _commandMgr->sendError(F("PINGREQ write failed"), F("MQTT Client"));
                    }
                }
            }
        }

        // Keep-alive timeout: no packet received within keep-alive + ping timeout window
        if (timeSinceLastReceive > (keepAliveMs + MqttPingTimeout))
        {
            if (_commandMgr != nullptr)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Keep-alive timeout: no packet for %lums (threshold=%lums)",
                    timeSinceLastReceive, keepAliveMs + MqttPingTimeout);
                _commandMgr->sendError(buf, F("MQTT Client"));
            }
            raiseEvent(MqttEvent::KeepAliveTimeout);
            setError(MqttError::Timeout);
            disconnect();
            return false;
        }
    }

    return true;
}

bool MQTTClient::publish(const char* topic, const char* payload, MqttQoS qos, bool retain)
{
    if (payload == nullptr)
    {
        return false;
    }
    
    return publish(topic, (const uint8_t*)payload, strlen(payload), qos, retain);
}

bool MQTTClient::publish(const char* topic, const uint8_t* payload, uint16_t length, MqttQoS qos, bool retain)
{
    if (_state != MqttConnectionState::Connected)
    {
        setError(MqttError::NotConnected);
        return false;
    }
    
    if (topic == nullptr || payload == nullptr)
    {
        setError(MqttError::InvalidParameter);
        return false;
    }
    
    if (length > MqttMaxPayloadLength)
    {
        setError(MqttError::BufferOverflow);
        return false;
    }
    
    // Basic validation: check topic/payload lengths against defined maxima
    if (strlen(topic) >= MqttMaxTopicLength)
    {
        setError(MqttError::BufferOverflow);
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("publish failed: topic exceeds MqttMaxTopicLength"), F("MQTT Client"));
        }
        return false;
    }

    // Copy payload to buffer
    memcpy(_payloadBuffer, payload, length);

    uint16_t packetLength = buildPublishPacket(_txBuffer, sizeof(_txBuffer),
        topic, _payloadBuffer, length, qos, retain);

    if (packetLength == 0)
    {
        raiseEvent(MqttEvent::BufferOverflow);
        setError(MqttError::BufferOverflow);
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("publish failed: buffer overflow"), F("MQTT Client"));
        }
        return false;
    }

    return writePacket(_txBuffer, packetLength);
}

bool MQTTClient::subscribe(const char* topic, MqttQoS qos)
{
    if (_state != MqttConnectionState::Connected)
    {
        setError(MqttError::NotConnected);
        return false;
    }
    
    if (topic == nullptr)
    {
        setError(MqttError::InvalidParameter);
        return false;
    }
    
    uint16_t packetLength = buildSubscribePacket(_txBuffer, sizeof(_txBuffer), topic, qos);

    if (packetLength == 0)
    {
        raiseEvent(MqttEvent::BufferOverflow);
        setError(MqttError::BufferOverflow);
        return false;
    }

    return writePacket(_txBuffer, packetLength);
}

bool MQTTClient::unsubscribe(const char* topic)
{
    if (_state != MqttConnectionState::Connected)
    {
        setError(MqttError::NotConnected);
        return false;
    }

    if (topic == nullptr)
    {
        setError(MqttError::InvalidParameter);
        return false;
    }

    uint16_t packetLength = buildUnsubscribePacket(_txBuffer, sizeof(_txBuffer), topic);

    if (packetLength == 0)
    {
        raiseEvent(MqttEvent::BufferOverflow);
        setError(MqttError::BufferOverflow);
        return false;
    }

    return writePacket(_txBuffer, packetLength);
}

uint32_t MQTTClient::getPacketsSent() const
{
    return _packetsSent;
}

uint32_t MQTTClient::getPacketsReceived() const
{
    return _packetsReceived;
}

void MQTTClient::resetStatistics()
{
    _packetsSent = 0;
    _packetsReceived = 0;
}

// ============================================================================
// Private Methods
// ============================================================================

uint16_t MQTTClient::getNextPacketId()
{
    _packetId++;
    if (_packetId >= MqttPacketIdMax)
    {
        _packetId = MqttPacketIdMin;
    }
    return _packetId;
}

uint8_t MQTTClient::encodeRemainingLength(uint8_t* buffer, uint32_t length)
{
    uint8_t bytes = 0;
    do
    {
        uint8_t byte = length % 128;
        length /= 128;
        if (length > 0)
        {
            byte |= MqttLengthContinuationBit;
        }
        buffer[bytes++] = byte;
    } while (length > 0);
    
    return bytes;
}

uint32_t MQTTClient::decodeRemainingLength(const uint8_t* buffer, uint8_t* bytes)
{
    uint32_t multiplier = 1;
    uint32_t value = 0;
    uint8_t encodedByte;
    *bytes = 0;
    
    do
    {
        encodedByte = buffer[*bytes];
        value += (encodedByte & MqttLengthValueMask) * multiplier;
        multiplier *= 128;
        (*bytes)++;
    } while ((encodedByte & MqttLengthContinuationBit) != 0);
    
    return value;
}

bool MQTTClient::writePacket(const uint8_t* buffer, uint16_t length)
{
    if (_wifiClient == nullptr || !_wifiClient->connected())
    {
        setError(MqttError::NetworkError);
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("writePacket failed: WiFi client not connected"), F("MQTT Client"));
        }
        return false;
    }
    
    size_t written = _wifiClient->write(buffer, length);
    if (written != length)
    {
        setError(MqttError::NetworkError);
        if (_commandMgr != nullptr)
        {
            char buf[48];
            snprintf(buf, sizeof(buf), "writePacket failed: wrote %u expected %u", (unsigned)written, length);
            _commandMgr->sendError(buf, F("MQTT Client"));
        }
        return false;
    }
    
    _lastSendTime = millis();
    _packetsSent++;
    
    return true;
}

bool MQTTClient::readPacket()
{
    // Read packet type
    if (_rxBufferPos == 0)
    {
        int byte = _wifiClient->read();
        if (byte < 0)
        {
            return false;
        }
        _rxBuffer[_rxBufferPos++] = (uint8_t)byte;
    }
    
    // Read remaining length
    // Uses MqttRxLengthNotDecoded (0xFFFF) as sentinel to distinguish
    // "not decoded yet" from a legitimately decoded length of 0 (e.g. PINGRESP).
    if (_rxPacketLength == MqttRxLengthNotDecoded)
    {
        bool lengthDecoded = false;
        while (_wifiClient->available() > 0 && _rxBufferPos < 5)
        {
            int byte = _wifiClient->read();
            if (byte < 0)
            {
                return false;
            }
            _rxBuffer[_rxBufferPos++] = (uint8_t)byte;

            // Check if this is the last length byte (MSB clear)
            if ((byte & MqttLengthContinuationBit) == 0)
            {
                uint8_t lengthBytes;
                _rxPacketLength = decodeRemainingLength(_rxBuffer + 1, &lengthBytes);
                if (_commandMgr != nullptr)
                {
                    char buf[48];
                    snprintf(buf, sizeof(buf), "Rx type: 0x%02X remaining: %u", _rxBuffer[0], _rxPacketLength);
                    _commandMgr->sendDebug(buf, F("MQTT Client"));
                }
                lengthDecoded = true;
                break;
            }
        }

        if (!lengthDecoded)
        {
            return false; // Need more data to finish reading length field
        }
    }
    
    // Read packet payload
    while (_wifiClient->available() > 0 && _rxBufferPos < (_rxPacketLength + 2))
    {
        int byte = _wifiClient->read();
        if (byte < 0)
        {
            return false;
        }
        _rxBuffer[_rxBufferPos++] = (uint8_t)byte;
    }
    
    // Check if full packet received
    uint8_t lengthBytes;
    decodeRemainingLength(_rxBuffer + 1, &lengthBytes);
    uint16_t totalLength = 1 + lengthBytes + _rxPacketLength;
    
    if (_rxBufferPos >= totalLength)
    {
        // Full packet received, process it
        uint8_t packetType = _rxBuffer[0] & 0xF0;
        const uint8_t* payload = _rxBuffer + 1 + lengthBytes;
        
        bool result = processPacket(packetType, payload, _rxPacketLength);
        
        // Reset for next packet
        _rxBufferPos = 0;
        _rxPacketLength = MqttRxLengthNotDecoded;
        
        _lastReceiveTime = millis();
        _packetsReceived++;
        
        return result;
    }
    
    return false; // Need more data
}

bool MQTTClient::processPacket(uint8_t packetType, const uint8_t* payload, uint16_t length)
{
    switch (packetType)
    {
        case static_cast<uint8_t>(MqttPacketType::ConnAck):
            return processConnAck(payload, length);
            
        case static_cast<uint8_t>(MqttPacketType::Publish):
            return processPublish(payload, length);
            
        case static_cast<uint8_t>(MqttPacketType::PingResp):
            return processPingResp(payload, length);
            
        default:
            // Unknown or unhandled packet type
            return true;
    }
}

uint16_t MQTTClient::buildConnectPacket(uint8_t* buffer, uint16_t bufferSize)
{
    (void)bufferSize;
    uint16_t pos = 0;
    
    // Fixed header
    buffer[pos++] = static_cast<uint8_t>(MqttPacketType::Connect);
    
    // Calculate remaining length (we'll fill this in later)
    uint16_t remainingLengthPos = pos;
    pos++; // Reserve space for remaining length byte(s)
    
    // Variable header
    // Protocol name
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x04;
    buffer[pos++] = 'M';
    buffer[pos++] = 'Q';
    buffer[pos++] = 'T';
    buffer[pos++] = 'T';
    
    // Protocol level
    buffer[pos++] = MqttProtocolVersion;
    
    // Connect flags
    uint8_t connectFlags = MqttConnFlagCleanSession;
    if (_useCredentials)
    {
        connectFlags |= MqttConnFlagUsername | MqttConnFlagPassword;
    }
    buffer[pos++] = connectFlags;
    
    // Keep alive
    buffer[pos++] = (_keepAlive >> 8) & 0xFF;
    buffer[pos++] = _keepAlive & 0xFF;
    
    // Payload
    // Client ID
    uint16_t clientIdLen = strlen(_clientId);
    buffer[pos++] = (clientIdLen >> 8) & 0xFF;
    buffer[pos++] = clientIdLen & 0xFF;
    memcpy(buffer + pos, _clientId, clientIdLen);
    pos += clientIdLen;
    
    // Username (if applicable)
    if (connectFlags & MqttConnFlagUsername)
    {
        uint16_t usernameLen = strlen(_username);
        buffer[pos++] = (usernameLen >> 8) & 0xFF;
        buffer[pos++] = usernameLen & 0xFF;
        memcpy(buffer + pos, _username, usernameLen);
        pos += usernameLen;
    }
    
    // Password (if applicable)
    if (connectFlags & MqttConnFlagPassword)
    {
        uint16_t passwordLen = strlen(_password);
        buffer[pos++] = (passwordLen >> 8) & 0xFF;
        buffer[pos++] = passwordLen & 0xFF;
        memcpy(buffer + pos, _password, passwordLen);
        pos += passwordLen;
    }
    
    // Fill in remaining length
    uint32_t remainingLength = pos - remainingLengthPos - 1;
    // Encode remaining length to a temporary buffer first to avoid overwriting payload
    uint8_t lenBuf[4];
    uint8_t lengthBytes = encodeRemainingLength(lenBuf, remainingLength);

    if (lengthBytes > 1)
    {
        // Shift existing payload forward to make room for extra length bytes
        memmove(buffer + remainingLengthPos + lengthBytes,
                buffer + remainingLengthPos + 1,
                remainingLength);
        // Copy encoded length bytes into position
        memcpy(buffer + remainingLengthPos, lenBuf, lengthBytes);
        pos += (lengthBytes - 1);
    }
    else
    {
        // Single-byte remaining length, write directly
        buffer[remainingLengthPos] = lenBuf[0];
    }
    
    return pos;
}

uint16_t MQTTClient::buildPublishPacket(uint8_t* buffer, uint16_t bufferSize,
    const char* topic, const char* payload, uint16_t payloadLen,
    MqttQoS qos, bool retain)
{
    (void)bufferSize;
    uint16_t pos = 0;
    
    // Fixed header
    uint8_t fixedHeader = static_cast<uint8_t>(MqttPacketType::Publish);
    if (retain)
    {
        fixedHeader |= MqttPublishFlagRetain;
    }
    if (qos == MqttQoS::AtLeastOnce)
    {
        fixedHeader |= MqttPublishFlagQoS1;
    }
    buffer[pos++] = fixedHeader;
    
    // Reserve space for remaining length
    uint16_t remainingLengthPos = pos;
    pos++;
    
    // Variable header
    // Topic name
    uint16_t topicLen = strlen(topic);
    buffer[pos++] = (topicLen >> 8) & 0xFF;
    buffer[pos++] = topicLen & 0xFF;
    memcpy(buffer + pos, topic, topicLen);
    pos += topicLen;
    
    // Packet identifier (for QoS > 0)
    if (qos != MqttQoS::AtMostOnce)
    {
        uint16_t packetId = getNextPacketId();
        buffer[pos++] = (packetId >> 8) & 0xFF;
        buffer[pos++] = packetId & 0xFF;
    }
    
    // Payload
    memcpy(buffer + pos, payload, payloadLen);
    pos += payloadLen;
    
    // Fill in remaining length
    uint32_t remainingLength = pos - remainingLengthPos - 1;
    uint8_t lenBuf[4];
    uint8_t lengthBytes = encodeRemainingLength(lenBuf, remainingLength);

    if (lengthBytes > 1)
    {
        memmove(buffer + remainingLengthPos + lengthBytes,
                buffer + remainingLengthPos + 1,
                remainingLength);
        memcpy(buffer + remainingLengthPos, lenBuf, lengthBytes);
        pos += (lengthBytes - 1);
    }
    else
    {
        buffer[remainingLengthPos] = lenBuf[0];
    }
    
    return pos;
}

uint16_t MQTTClient::buildSubscribePacket(uint8_t* buffer, uint16_t bufferSize,
    const char* topic, MqttQoS qos)
{
    (void)bufferSize;
    uint16_t pos = 0;
    
    // Fixed header
    buffer[pos++] = static_cast<uint8_t>(MqttPacketType::Subscribe) | MqttSubscribeFlags;
    
    // Reserve space for remaining length
    uint16_t remainingLengthPos = pos;
    pos++;
    
    // Variable header
    // Packet identifier
    uint16_t packetId = getNextPacketId();
    buffer[pos++] = (packetId >> 8) & 0xFF;
    buffer[pos++] = packetId & 0xFF;
    
    // Payload
    // Topic filter
    uint16_t topicLen = strlen(topic);
    buffer[pos++] = (topicLen >> 8) & 0xFF;
    buffer[pos++] = topicLen & 0xFF;
    memcpy(buffer + pos, topic, topicLen);
    pos += topicLen;
    
    // Requested QoS
    buffer[pos++] = static_cast<uint8_t>(qos);
    
    // Fill in remaining length
    uint32_t remainingLength = pos - remainingLengthPos - 1;
    uint8_t lenBuf[4];
    uint8_t lengthBytes = encodeRemainingLength(lenBuf, remainingLength);

    if (lengthBytes > 1)
    {
        memmove(buffer + remainingLengthPos + lengthBytes,
                buffer + remainingLengthPos + 1,
                remainingLength);
        memcpy(buffer + remainingLengthPos, lenBuf, lengthBytes);
        pos += (lengthBytes - 1);
    }
    else
    {
        buffer[remainingLengthPos] = lenBuf[0];
    }
    
    return pos;
}

uint16_t MQTTClient::buildUnsubscribePacket(uint8_t* buffer, uint16_t bufferSize,
    const char* topic)
{
    (void)bufferSize;
    uint16_t pos = 0;

    // Fixed header
    buffer[pos++] = static_cast<uint8_t>(MqttPacketType::Unsubscribe) | MqttUnsubscribeFlags;

    // Reserve space for remaining length
    uint16_t remainingLengthPos = pos;
    pos++;

    // Variable header
    // Packet identifier
    uint16_t packetId = getNextPacketId();
    buffer[pos++] = (packetId >> 8) & 0xFF;
    buffer[pos++] = packetId & 0xFF;

    // Payload
    // Topic filter
    uint16_t topicLen = strlen(topic);
    buffer[pos++] = (topicLen >> 8) & 0xFF;
    buffer[pos++] = topicLen & 0xFF;
    memcpy(buffer + pos, topic, topicLen);
    pos += topicLen;

    // Fill in remaining length
    uint32_t remainingLength = pos - remainingLengthPos - 1;
    uint8_t lenBuf[4];
    uint8_t lengthBytes = encodeRemainingLength(lenBuf, remainingLength);

    if (lengthBytes > 1)
    {
        memmove(buffer + remainingLengthPos + lengthBytes,
                buffer + remainingLengthPos + 1,
                remainingLength);
        memcpy(buffer + remainingLengthPos, lenBuf, lengthBytes);
        pos += (lengthBytes - 1);
    }
    else
    {
        buffer[remainingLengthPos] = lenBuf[0];
    }

    return pos;
}

uint16_t MQTTClient::buildPingReqPacket(uint8_t* buffer, uint16_t bufferSize)
{
    (void)bufferSize;
    buffer[0] = static_cast<uint8_t>(MqttPacketType::PingReq);
    buffer[1] = 0x00; // Remaining length
    return 2;
}

uint16_t MQTTClient::buildDisconnectPacket(uint8_t* buffer, uint16_t bufferSize)
{
    (void)bufferSize;
    buffer[0] = static_cast<uint8_t>(MqttPacketType::Disconnect);
    buffer[1] = 0x00; // Remaining length
    return 2;
}

bool MQTTClient::processConnAck(const uint8_t* payload, uint16_t length)
{
    if (length < 2)
    {
        raiseEvent(MqttEvent::InvalidPacket);
        setError(MqttError::InvalidPacket);
        return false;
    }

    uint8_t returnCode = payload[1];

    if (returnCode == static_cast<uint8_t>(MqttConnectReturnCode::Accepted))
    {
        raiseEvent(MqttEvent::Connected);
        _state = MqttConnectionState::Connected;
        _lastReceiveTime = millis();

        if (_connectionCallback != nullptr)
        {
            _connectionCallback(true);
        }

        return true;
    }
    else
    {
        raiseEvent(MqttEvent::ConnectionRefused, returnCode);
        setError(MqttError::ProtocolError);
        disconnect();
        return false;
    }
}

bool MQTTClient::processPublish(const uint8_t* payload, uint16_t length)
{
    if (length < 2)
    {
        raiseEvent(MqttEvent::InvalidPacket);
        return false;
    }

    // Extract topic
    uint16_t topicLen = (payload[0] << 8) | payload[1];
    if (topicLen > MqttMaxTopicLength - 1 || topicLen + 2 > length)
    {
        raiseEvent(MqttEvent::InvalidPacket);
        return false;
    }

    memcpy(_topicBuffer, payload + 2, topicLen);
    _topicBuffer[topicLen] = '\0';

    // Extract payload
    uint16_t payloadStart = 2 + topicLen;
    uint16_t payloadLength = length - payloadStart;

    if (payloadLength > MqttMaxPayloadLength - 1)
    {
        payloadLength = MqttMaxPayloadLength - 1;
    }

    memcpy(_payloadBuffer, payload + payloadStart, payloadLength);
    _payloadBuffer[payloadLength] = '\0';

    // Call callback
    if (_messageCallback != nullptr)
    {
        _messageCallback(_topicBuffer, _payloadBuffer, payloadLength);
    }

    return true;
}

bool MQTTClient::processPingResp(const uint8_t* payload, uint16_t length)
{
    (void)payload;
    (void)length;
    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(F("PINGRESP received"), F("MQTT Client"));
    }
    return true;
}

void MQTTClient::resetBuffers()
{
    memset(_rxBuffer, 0, sizeof(_rxBuffer));
    memset(_txBuffer, 0, sizeof(_txBuffer));
    memset(_topicBuffer, 0, sizeof(_topicBuffer));
    memset(_payloadBuffer, 0, sizeof(_payloadBuffer));
}

void MQTTClient::setError(MqttError error)
{
    _lastError = error;
}

void MQTTClient::raiseEvent(MqttEvent event, uint8_t errorCode)
{
    if (_eventCallback != nullptr)
    {
        _eventCallback(event, errorCode);
    }
}
