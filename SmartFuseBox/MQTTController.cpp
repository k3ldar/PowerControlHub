#include "Local.h"
#include "MQTTController.h"
#include "Config.h"
#include <SerialCommandManager.h>
#include <stdio.h>


// Static instance pointer for callbacks
MQTTController* MQTTController::_instance = nullptr;

MQTTController::MQTTController(MessageBus* messageBus, Config* config, SerialCommandManager* commandMgr)
    : _mqttClient(nullptr)
    , _messageBus(messageBus)
    , _config(config)
    , _retryCount(0)
    , _lastRetryTime(0)
    , _isEnabled(false)
    , _commandMgr(commandMgr)
    , _connectedSince(0)
    , _reconnectCount(0)
{
    _instance = this;
}

MQTTController::~MQTTController()
{
    end();
    _instance = nullptr;
}

bool MQTTController::begin()
{
    if (_config == nullptr || _messageBus == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Config or MessageBus not available"), F("MQTT Controller"));
        }
        return false;
    }

    // Check if MQTT is enabled in config
    if (!_config->mqtt.enabled)
    {
        _isEnabled = false;
        return false;
    }

    // Create MQTT client
    if (_mqttClient == nullptr)
    {
        _mqttClient = new MQTTClient();
    }

    // Configure client from config
    _mqttClient->setBroker(_config->mqtt.broker, _config->mqtt.port);

    if (_config->mqtt.username[0] != '\0' && _config->mqtt.password[0] != '\0')
    {
        _mqttClient->setCredentials(_config->mqtt.username, _config->mqtt.password);
    }

    _mqttClient->setClientId(_config->mqtt.deviceId);
    _mqttClient->setKeepAlive(_config->mqtt.keepAliveInterval);

    // Set callbacks
    _mqttClient->setCommandManager(_commandMgr);
    _mqttClient->setConnectionCallback(connectionCallbackStatic);
    _mqttClient->setMessageCallback(messageCallbackStatic);
    _mqttClient->setEventCallback(eventCallbackStatic);

    _isEnabled = true;

    // Attempt initial connection (but don't fail initialization if connection fails)
    connect();

    // Return true if MQTT is configured and enabled
    // Actual connection will be handled by retry logic in update()
    return true;
}

void MQTTController::update()
{
    if (!_isEnabled || _mqttClient == nullptr)
    {
        return;
    }

    // Update MQTT client (non-blocking)
    _mqttClient->update();

    // Handle reconnection logic - only retry if we're fully disconnected, not if we're already connecting
    MqttConnectionState state = _mqttClient->getState();
    if (state == MqttConnectionState::Disconnected && shouldRetry())
    {
        _lastRetryTime = millis();
        connect();
    }
}

void MQTTController::end()
{
    if (_mqttClient != nullptr)
    {
        _mqttClient->disconnect();
        delete _mqttClient;
        _mqttClient = nullptr;
    }
    
    _isEnabled = false;
}

bool MQTTController::connect()
{
    if (_mqttClient == nullptr || !_isEnabled)
    {
        return false;
    }

    if (_mqttClient->isConnected())
    {
        return true;
    }

    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(F("Attempting connect"), F("MQTT Controller"));
    }
    bool result = _mqttClient->connect();

    if (result)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendDebug(F("CONNECT packet sent"), F("MQTT Controller"));
        }
    }
    else
    {
        if (_commandMgr != nullptr)
        {
            char buf[40];
            snprintf(buf, sizeof(buf), "connect() failed, error: %d", static_cast<int>(_mqttClient->getLastError()));
            _commandMgr->sendError(buf, F("MQTT Controller"));
        }
    }

    return result;
}

void MQTTController::disconnect()
{
    if (_mqttClient != nullptr)
    {
        _mqttClient->disconnect();
    }
    
    _retryCount = 0;
}

bool MQTTController::isConnected() const
{
    if (_mqttClient == nullptr)
    {
        return false;
    }
    
    return _mqttClient->isConnected();
}

bool MQTTController::isEnabled() const
{
    return _isEnabled;
}

void MQTTController::setEnabled(bool enabled)
{
    _isEnabled = enabled;
    
    if (!enabled && _mqttClient != nullptr)
    {
        disconnect();
    }
}

bool MQTTController::publishState(const char* topic, const char* value)
{
    if (!isConnected() || topic == nullptr || value == nullptr)
    {
        return false;
    }

    // Use retain=true for state topics so subscribers get last known state
    return _mqttClient->publish(topic, value, MqttQoS::AtMostOnce, true);
}

uint32_t MQTTController::getReconnectCount() const
{
    return _reconnectCount;
}

unsigned long MQTTController::getUptime() const
{
    if (!isConnected() || _connectedSince == 0)
    {
        return 0;
    }
    
    return (millis() - _connectedSince) / 1000; // Convert to seconds
}

MQTTClient* MQTTController::getClient()
{
    return _mqttClient;
}

// ============================================================================
// Private Methods
// ============================================================================

void MQTTController::onMqttConnected(bool connected)
{
    if (connected)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendDebug(F("Connected"), F("MQTT Controller"));
        }
        _retryCount = 0;
        _connectedSince = millis();
        _reconnectCount++;

#if defined(MQTT_SUPPORT)
        // Publish to MessageBus
        if (_messageBus != nullptr)
        {
            _messageBus->publish<MqttConnected>();
        }
#endif
    }
    else
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendDebug(F("Disconnected"), F("MQTT Controller"));
        }
        _connectedSince = 0;

        // Increment retry count on disconnection (includes timeouts and connection failures)
        _retryCount++;

        // Publish to MessageBus
        if (_messageBus != nullptr)
        {
#if defined(MQTT_SUPPORT)
            _messageBus->publish<MqttDisconnected>();
#endif
        }
    }
}

void MQTTController::onMqttMessage(const char* topic, const char* payload, uint16_t length)
{
    (void)length;
    // Publish to MessageBus for routing
    if (_messageBus != nullptr)
    {
#if defined(MQTT_SUPPORT)
        _messageBus->publish<MqttMessageReceived>(topic, payload);
#else
        (void)topic;
        (void)payload;
#endif
    }
}

void MQTTController::onMqttEvent(MqttEvent event, uint8_t errorCode)
{
    // Handle MQTT events and log them as appropriate
    switch (event)
    {
        case MqttEvent::Connected:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendDebug(F("Connected"), F("MQTT"));
            }
            break;

        case MqttEvent::Disconnected:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendDebug(F("Disconnected"), F("MQTT"));
            }
            break;

        case MqttEvent::ConnectionRefused:
            if (_commandMgr != nullptr)
            {
                char buf[40];
                snprintf(buf, sizeof(buf), "Connection refused, code: %u", errorCode);
                _commandMgr->sendError(buf, F("MQTT"));
            }
            break;

        case MqttEvent::ConnectionTimeout:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Connection timeout"), F("MQTT"));
            }
            break;

        case MqttEvent::BrokerNotConfigured:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("No broker configured"), F("MQTT"));
            }
            break;

        case MqttEvent::BufferOverflow:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Buffer overflow"), F("MQTT"));
            }
            break;

        case MqttEvent::InvalidPacket:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Invalid packet"), F("MQTT"));
            }
            break;

        case MqttEvent::NetworkError:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Network error"), F("MQTT"));
            }
            break;

        case MqttEvent::KeepAliveTimeout:
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Keep-alive timeout"), F("MQTT"));
            }
            break;

        default:
            // Ignore other events (PacketSent, PacketReceived, etc.)
            break;
    }
}

bool MQTTController::shouldRetry()
{
    if (_retryCount >= MqttMaxRetries)
    {
        if (_retryCount == MqttMaxRetries)
        {
            if (_commandMgr != nullptr)
            {
                _commandMgr->sendError(F("Max retry attempts reached"), F("MQTT"));
            }
            _retryCount++; // Increment to prevent this message from repeating
        }
        return false;
    }

    unsigned long now = millis();
    uint16_t delay = getRetryDelay();
    unsigned long elapsed = now - _lastRetryTime;

    return elapsed >= delay;
}

uint16_t MQTTController::getRetryDelay()
{
    if (_retryCount >= MqttMaxRetries)
    {
        return MqttRetryDelays[MqttMaxRetries - 1];
    }
    
    return MqttRetryDelays[_retryCount];
}

// ============================================================================
// Static Callbacks
// ============================================================================

void MQTTController::connectionCallbackStatic(bool connected)
{
    if (_instance != nullptr)
    {
        _instance->onMqttConnected(connected);
    }
}

void MQTTController::messageCallbackStatic(const char* topic, const char* payload, uint16_t length)
{
    if (_instance != nullptr)
    {
        _instance->onMqttMessage(topic, payload, length);
    }
}

void MQTTController::eventCallbackStatic(MqttEvent event, uint8_t errorCode)
{
    if (_instance != nullptr)
    {
        _instance->onMqttEvent(event, errorCode);
    }
}
