#include "Local.h"

#if defined(MQTT_SUPPORT)

#include "MQTTSensorHandler.h"
#include "SystemDefinitions.h"
#include <SerialCommandManager.h>
#include <string.h>
#include <stdio.h>

MQTTSensorHandler::MQTTSensorHandler(MQTTController* mqttController, MessageBus* messageBus, SensorController* sensorController, SerialCommandManager* commandMgr)
    : MQTTHandler(mqttController, messageBus)
    , _config(nullptr)
    , _sensorController(sensorController)
    , _commandMgr(commandMgr)
    , _totalChannels(0)
    , _tempChannelIdx(-1)
    , _humidityChannelIdx(-1)
    , _lightChannelIdx(-1)
    , _waterChannelIdx(-1)
    , _discoveryPending(false)
    , _discoveryIndex(0)
    , _lastDiscoveryPublish(0)
    , _lastStatePublish(0)
{
    _config = ConfigManager::getConfigPtr();
    memset(_channelMap, 0, sizeof(_channelMap));
    memset(_dirtyChannels, 0, sizeof(_dirtyChannels));
}

bool MQTTSensorHandler::begin()
{
    if (_mqttController == nullptr || _messageBus == nullptr || _config == nullptr || _sensorController == nullptr)
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Required components not available"), F("MQTT Sensor"));
        }
        return false;
    }

    _messageBus->subscribe<TemperatureUpdated>(
        [this](float temp)
        {
            this->onTemperatureUpdated(temp);
        }
    );

    _messageBus->subscribe<HumidityUpdated>(
        [this](uint8_t humidity)
        {
            this->onHumidityUpdated(humidity);
        }
    );

    _messageBus->subscribe<LightSensorUpdated>(
        [this](bool isDaytime)
        {
            this->onLightSensorUpdated(isDaytime);
        }
    );

    _messageBus->subscribe<WaterLevelUpdated>(
        [this](uint16_t waterLevel, uint16_t averageWaterLevel)
        {
            this->onWaterLevelUpdated(waterLevel, averageWaterLevel);
        }
    );

    // Build channel map from sensor controller
    _totalChannels = 0;
    for (uint8_t s = 0; s < _sensorController->sensorCount() && _totalChannels < MaxMqttSensorChannels; s++)
    {
        BaseSensor* sensor = _sensorController->sensorGet(s);
        if (sensor == nullptr)
        {
            continue;
        }

        for (uint8_t c = 0; c < sensor->getMqttChannelCount() && _totalChannels < MaxMqttSensorChannels; c++)
        {
            _channelMap[_totalChannels].sensor = sensor;
            _channelMap[_totalChannels].channelIndex = c;

            const char* slug = sensor->getMqttChannel(c).slug;
            if (strcmp(slug, "temperature") == 0)
            {
                _tempChannelIdx = static_cast<int8_t>(_totalChannels);
            }
            else if (strcmp(slug, "humidity") == 0)
            {
                _humidityChannelIdx = static_cast<int8_t>(_totalChannels);
            }
            else if (strcmp(slug, "light") == 0)
            {
                _lightChannelIdx = static_cast<int8_t>(_totalChannels);
            }
            else if (strcmp(slug, "water_level") == 0)
            {
                _waterChannelIdx = static_cast<int8_t>(_totalChannels);
            }

            _totalChannels++;
        }
    }

    _messageBus->subscribe<MqttConnected>(
        [this]()
        {
            this->subscribe();

            if (_config != nullptr && _config->mqtt.useHomeAssistantDiscovery)
            {
                _discoveryPending = true;
                _discoveryIndex = 0;
            }

            this->markAllChannelsDirty();
        }
    );

    return true;
}

void MQTTSensorHandler::update()
{
    unsigned long now = millis();

    if (_discoveryPending && _discoveryIndex < _totalChannels)
    {
        if (_mqttController != nullptr && _mqttController->isConnected())
        {
            if (now - _lastDiscoveryPublish >= MinPublishInterval)
            {
                publishSensorDiscoveryConfig(_discoveryIndex);
                _lastDiscoveryPublish = now;
                _discoveryIndex++;

                if (_discoveryIndex >= _totalChannels)
                {
                    if (_commandMgr != nullptr)
                    {
                        _commandMgr->sendDebug(F("Sensor discovery complete"), F("MQTT"));
                    }
                    _discoveryPending = false;
                }
            }
        }
    }

    bool anyDirty = false;
    for (uint8_t i = 0; i < _totalChannels; i++)
    {
        if (_dirtyChannels[i])
        {
            anyDirty = true;
            break;
        }
    }

    if (anyDirty)
    {
        unsigned long timeSinceLastPublish = now - _lastStatePublish;

        if (timeSinceLastPublish >= MinPublishInterval || timeSinceLastPublish >= MaxPublishInterval)
        {
            publishDirtySensorStates();
        }
    }
}

void MQTTSensorHandler::end()
{
    unsubscribe();
    _isSubscribed = false;
}

void MQTTSensorHandler::onMessage(const char* topic, const char* payload)
{
    // Sensors are read-only; no commands expected
    (void)topic;
    (void)payload;
}

bool MQTTSensorHandler::subscribe()
{
    // Sensors publish only; no MQTT topics to subscribe to
    _isSubscribed = true;
    return true;
}

void MQTTSensorHandler::unsubscribe()
{
    _isSubscribed = false;
}

// ============================================================================
// Private Methods
// ============================================================================

void MQTTSensorHandler::onTemperatureUpdated(float temperature)
{
    (void)temperature;
    markChannelDirty(_tempChannelIdx);
}

void MQTTSensorHandler::onHumidityUpdated(uint8_t humidity)
{
    (void)humidity;
    markChannelDirty(_humidityChannelIdx);
}

void MQTTSensorHandler::onLightSensorUpdated(bool isDaytime)
{
    (void)isDaytime;
    markChannelDirty(_lightChannelIdx);
}

void MQTTSensorHandler::onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel)
{
    (void)waterLevel;
    (void)averageWaterLevel;
    markChannelDirty(_waterChannelIdx);
}

void MQTTSensorHandler::publishSensorState(uint8_t channelIndex)
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        return;
    }

    if (!_mqttController->isConnected())
    {
        return;
    }

    if (channelIndex >= _totalChannels)
    {
        return;
    }

    const MqttChannelMap& entry = _channelMap[channelIndex];
    MqttSensorChannel channel = entry.sensor->getMqttChannel(entry.channelIndex);

    char topic[MqttMaxTopicLength];
    char payload[32];

    snprintf(topic, sizeof(topic), "home/%s/sensor/%s/state", _config->mqtt.deviceId, channel.slug);
    entry.sensor->getMqttValue(entry.channelIndex, payload, sizeof(payload));

    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(topic, F("MQTT Sensor State"));
    }

    _mqttController->publishState(topic, payload);
}

void MQTTSensorHandler::publishDirtySensorStates()
{
    if (_mqttController == nullptr)
    {
        return;
    }

    if (!_mqttController->isConnected())
    {
        return;
    }

    for (uint8_t i = 0; i < _totalChannels; i++)
    {
        if (_dirtyChannels[i])
        {
            publishSensorState(i);
        }
    }

    memset(_dirtyChannels, 0, sizeof(_dirtyChannels));
    _lastStatePublish = millis();
}

void MQTTSensorHandler::markChannelDirty(int8_t channelIndex)
{
    if (channelIndex >= 0 && channelIndex < static_cast<int8_t>(MaxMqttSensorChannels))
    {
        _dirtyChannels[channelIndex] = true;
    }
}

void MQTTSensorHandler::markAllChannelsDirty()
{
    for (uint8_t i = 0; i < _totalChannels; i++)
    {
        _dirtyChannels[i] = true;
    }
}

void MQTTSensorHandler::publishDiscoveryConfig()
{
    _discoveryPending = true;
    _discoveryIndex = 0;
}

void MQTTSensorHandler::publishSensorDiscoveryConfig(uint8_t index)
{
    if (_mqttController == nullptr || _config == nullptr)
    {
        return;
    }

    if (!_mqttController->isConnected())
    {
        return;
    }

    if (index >= _totalChannels)
    {
        return;
    }

    MQTTClient* client = _mqttController->getClient();
    if (client == nullptr)
    {
        return;
    }

    const MqttChannelMap& entry = _channelMap[index];
    MqttSensorChannel channel = entry.sensor->getMqttChannel(entry.channelIndex);

    char topic[MqttMaxTopicLength];
    char payload[MqttMaxPayloadLength];

    const char* entityType = channel.isBinary ? "binary_sensor" : "sensor";

    snprintf(topic, sizeof(topic), "%s/%s/%s/%s/config",
        _config->mqtt.discoveryPrefix, entityType, _config->mqtt.deviceId, channel.slug);

    if (channel.isBinary)
    {
        if (channel.deviceClass != nullptr)
        {
            snprintf(payload, sizeof(payload),
                "{\"name\":\"%s\","
                "\"state_topic\":\"home/%s/sensor/%s/state\","
                "\"device_class\":\"%s\","
                "\"payload_on\":\"ON\","
                "\"payload_off\":\"OFF\","
                "\"unique_id\":\"%s_%s\","
                "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
                channel.name,
                _config->mqtt.deviceId, channel.slug,
                channel.deviceClass,
                _config->mqtt.deviceId, channel.slug,
                _config->mqtt.deviceId);
        }
        else
        {
            snprintf(payload, sizeof(payload),
                "{\"name\":\"%s\","
                "\"state_topic\":\"home/%s/sensor/%s/state\","
                "\"payload_on\":\"ON\","
                "\"payload_off\":\"OFF\","
                "\"unique_id\":\"%s_%s\","
                "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
                channel.name,
                _config->mqtt.deviceId, channel.slug,
                _config->mqtt.deviceId, channel.slug,
                _config->mqtt.deviceId);
        }
    }
    else if (channel.deviceClass != nullptr && channel.unit != nullptr)
    {
        snprintf(payload, sizeof(payload),
            "{\"name\":\"%s\","
            "\"state_topic\":\"home/%s/sensor/%s/state\","
            "\"device_class\":\"%s\","
            "\"unit_of_measurement\":\"%s\","
            "\"unique_id\":\"%s_%s\","
            "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
            channel.name,
            _config->mqtt.deviceId, channel.slug,
            channel.deviceClass,
            channel.unit,
            _config->mqtt.deviceId, channel.slug,
            _config->mqtt.deviceId);
    }
    else if (channel.deviceClass != nullptr)
    {
        snprintf(payload, sizeof(payload),
            "{\"name\":\"%s\","
            "\"state_topic\":\"home/%s/sensor/%s/state\","
            "\"device_class\":\"%s\","
            "\"unique_id\":\"%s_%s\","
            "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
            channel.name,
            _config->mqtt.deviceId, channel.slug,
            channel.deviceClass,
            _config->mqtt.deviceId, channel.slug,
            _config->mqtt.deviceId);
    }
    else
    {
        snprintf(payload, sizeof(payload),
            "{\"name\":\"%s\","
            "\"state_topic\":\"home/%s/sensor/%s/state\","
            "\"unique_id\":\"%s_%s\","
            "\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}",
            channel.name,
            _config->mqtt.deviceId, channel.slug,
            _config->mqtt.deviceId, channel.slug,
            _config->mqtt.deviceId);
    }

    client->publish(topic, payload, MqttQoS::AtMostOnce, true);
}

#endif