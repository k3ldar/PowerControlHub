#include "Local.h"
#include "MQTTSensorHandler.h"
#include "SystemDefinitions.h"
#include <SerialCommandManager.h>
#include <string.h>
#include <stdio.h>

const char PROGMEM JsonBase[] = "{\"name\":\"%s\",\"state_topic\":\"home/%s/sensor/%s/state\"";
const char PROGMEM JsonBinaryPayload[] = ",\"payload_on\":\"ON\",\"payload_off\":\"OFF\"";
const char PROGMEM JsonDeviceClass[] = ",\"device_class\":\"%s\"";
const char PROGMEM JsonUnit[] = ",\"unit_of_measurement\":\"%s\"";
const char PROGMEM JsonFooter[] = ",\"unique_id\":\"%s_%s\",\"device\":{\"ids\":[\"%s\"],\"name\":\"Smart Fuse Box\",\"mf\":\"Simon Carter\",\"mdl\":\"SFB v1\"}}";
const char PROGMEM DiscoveryTopicFormat[] = "%s/%s/%s/%s/config";
const char PROGMEM StateTopicFormat[] = "home/%s/sensor/%s/state";
const char PROGMEM EntityTypeBinarySensor[] = "binary_sensor";
const char PROGMEM EntityTypeSensor[] = "sensor";

MQTTSensorHandler::MQTTSensorHandler(MQTTController* mqttController, MessageBus* messageBus, SensorController* sensorController, SerialCommandManager* commandMgr)
    : MQTTHandler(mqttController, messageBus)
    , _config(nullptr)
    , _sensorController(sensorController)
    , _commandMgr(commandMgr)
    , _tempChannelIdx(-1)
    , _humidityChannelIdx(-1)
    , _lightChannelIdx(-1)
    , _waterChannelIdx(-1)
    , _discoveryPending(false)
    , _discoveryIndex(0)
    , _lastDiscoveryPublish(0)
{
    _config = ConfigManager::getConfigPtr();
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
    _channelMap.clear();
    for (uint8_t s = 0; s < _sensorController->sensorCount(); s++)
    {
        BaseSensor* sensor = _sensorController->sensorGet(s);
        if (sensor == nullptr)
        {
            continue;
        }

        for (uint8_t c = 0; c < sensor->getMqttChannelCount(); c++)
        {
            _channelMap.push_back({ sensor, c });

            uint8_t newIdx = static_cast<uint8_t>(_channelMap.size() - 1);
            const char* slug = sensor->getMqttChannel(c).slug;
            if (strcmp(slug, "temperature") == 0)
            {
                _tempChannelIdx = static_cast<int8_t>(newIdx);
            }
            else if (strcmp(slug, "humidity") == 0)
            {
                _humidityChannelIdx = static_cast<int8_t>(newIdx);
            }
            else if (strcmp(slug, "light") == 0)
            {
                _lightChannelIdx = static_cast<int8_t>(newIdx);
            }
            else if (strcmp(slug, "water_level") == 0)
            {
                _waterChannelIdx = static_cast<int8_t>(newIdx);
            }
        }
    }

    _messageBus->subscribe<SystemMetricsUpdated>(
        [this]()
        {
            for (uint8_t i = 0; i < _channelMap.size(); i++)
            {
                this->publishSensorState(i);
            }
        }
    );

    _messageBus->subscribe<MqttConnected>(
        [this]()
        {
            this->subscribe();

            if (_config != nullptr && _config->mqtt.useHomeAssistantDiscovery)
            {
                _discoveryPending = true;
                _discoveryIndex = 0;
            }

            for (uint8_t i = 0; i < _channelMap.size(); i++)
            {
                this->publishSensorState(i);
            }
        }
    );

    return true;
}

void MQTTSensorHandler::update()
{
    unsigned long now = millis();

    if (_discoveryPending && _discoveryIndex < _channelMap.size())
    {
        if (_mqttController != nullptr && _mqttController->isConnected())
        {
            if (now - _lastDiscoveryPublish >= MinPublishInterval)
            {
                publishSensorDiscoveryConfig(_discoveryIndex);
                _lastDiscoveryPublish = now;
                _discoveryIndex++;

                if (_discoveryIndex >= _channelMap.size())
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
    if (_tempChannelIdx >= 0)
    {
        publishSensorState(static_cast<uint8_t>(_tempChannelIdx));
    }
}

void MQTTSensorHandler::onHumidityUpdated(uint8_t humidity)
{
    (void)humidity;
    if (_humidityChannelIdx >= 0)
    {
        publishSensorState(static_cast<uint8_t>(_humidityChannelIdx));
    }
}

void MQTTSensorHandler::onLightSensorUpdated(bool isDaytime)
{
    (void)isDaytime;
    if (_lightChannelIdx >= 0)
    {
        publishSensorState(static_cast<uint8_t>(_lightChannelIdx));
    }
}

void MQTTSensorHandler::onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel)
{
    (void)waterLevel;
    (void)averageWaterLevel;
    if (_waterChannelIdx >= 0)
    {
        publishSensorState(static_cast<uint8_t>(_waterChannelIdx));
    }
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

    if (channelIndex >= _channelMap.size())
    {
        return;
    }

    const MqttChannelMap& entry = _channelMap[channelIndex];
    MqttSensorChannel channel = entry.sensor->getMqttChannel(entry.channelIndex);

    char topic[MqttMaxTopicLength];
    char payload[32];

    snprintf_P(topic, sizeof(topic), StateTopicFormat, _config->mqtt.deviceId, channel.slug);
    entry.sensor->getMqttValue(entry.channelIndex, payload, sizeof(payload));

    if (_commandMgr != nullptr)
    {
        _commandMgr->sendDebug(topic, F("MQTT Sensor State"));
    }

    _mqttController->publishState(topic, payload);
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

    if (index >= _channelMap.size())
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

    const char* entityType = channel.isBinary ? EntityTypeBinarySensor : EntityTypeSensor;

    snprintf_P(topic, sizeof(topic), DiscoveryTopicFormat,
        _config->mqtt.discoveryPrefix, entityType, _config->mqtt.deviceId, channel.slug);

    int offset = snprintf_P(payload, sizeof(payload), JsonBase,
        channel.name,
        _config->mqtt.deviceId, channel.slug);

    if (channel.isBinary)
    {
        offset += snprintf_P(payload + offset, sizeof(payload) - offset, JsonBinaryPayload);
    }

    if (channel.deviceClass != nullptr)
    {
        offset += snprintf_P(payload + offset, sizeof(payload) - offset, JsonDeviceClass,
            channel.deviceClass);
    }

    if (channel.unit != nullptr)
    {
        offset += snprintf_P(payload + offset, sizeof(payload) - offset, JsonUnit,
            channel.unit);
    }

    snprintf_P(payload + offset, sizeof(payload) - offset, JsonFooter,
        _config->mqtt.deviceId, channel.slug,
        _config->mqtt.deviceId);

    client->publish(topic, payload, MqttQoS::AtMostOnce, true);
}
