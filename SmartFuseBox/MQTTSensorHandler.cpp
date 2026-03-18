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
#include "Local.h"

#if defined(MQTT_SUPPORT)
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
    : MQTTHandler(mqttController, messageBus),
        _config(nullptr),
        _sensorController(sensorController),
        _commandMgr(commandMgr),
        _discoveryPending(false),
        _discoveryIndex(0),
        _lastDiscoveryPublish(0)
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
        [this](bool isDaytime, uint16_t lightLevel, uint16_t averageLightLevel)
        {
            this->onLightSensorUpdated(isDaytime, lightLevel, averageLightLevel);
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

    for (uint8_t i = 0; i < _channelMap.size(); i++)
    {
        MqttSensorChannel ch = _channelMap[i].sensor->getMqttChannel(_channelMap[i].channelIndex);

        if (ch.typeSlug != nullptr && strcmp(ch.typeSlug, "temperature") == 0)
        {
            publishSensorState(i);
        }
    }
}

void MQTTSensorHandler::onHumidityUpdated(uint8_t humidity)
{
    (void)humidity;

    for (uint8_t i = 0; i < _channelMap.size(); i++)
    {
        MqttSensorChannel ch = _channelMap[i].sensor->getMqttChannel(_channelMap[i].channelIndex);

        if (ch.typeSlug != nullptr && strcmp(ch.typeSlug, "humidity") == 0)
        {
            publishSensorState(i);
        }
    }
}

void MQTTSensorHandler::onLightSensorUpdated(bool isDaytime, uint16_t lightLevel, uint16_t averageLightLevel)
{
    (void)isDaytime;
    (void)lightLevel;
    (void)averageLightLevel;

    for (uint8_t i = 0; i < _channelMap.size(); i++)
    {
        MqttSensorChannel ch = _channelMap[i].sensor->getMqttChannel(_channelMap[i].channelIndex);

        if (ch.typeSlug != nullptr &&
            (strcmp(ch.typeSlug, "light") == 0 ||
             strcmp(ch.typeSlug, "light_level") == 0 ||
             strcmp(ch.typeSlug, "avg_light_level") == 0))
        {
            publishSensorState(i);
        }
    }
}

void MQTTSensorHandler::onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel)
{
    (void)waterLevel;
    (void)averageWaterLevel;

    for (uint8_t i = 0; i < _channelMap.size(); i++)
    {
        MqttSensorChannel ch = _channelMap[i].sensor->getMqttChannel(_channelMap[i].channelIndex);

        if (ch.typeSlug != nullptr && strcmp(ch.typeSlug, "water_level") == 0)
        {
            publishSensorState(i);
        }
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

    int topicLen = snprintf_P(topic, sizeof(topic), DiscoveryTopicFormat,
        _config->mqtt.discoveryPrefix, entityType, _config->mqtt.deviceId, channel.slug);
    if (topicLen < 0 || topicLen >= static_cast<int>(sizeof(topic)))
    {
        if (_commandMgr != nullptr)
        {
            _commandMgr->sendError(F("Discovery topic truncated"), F("MQTT Sensor"));
        }
        return;
    }

    // payload[512]
    size_t offset = 0;
    size_t remaining = sizeof(payload);

    int ret = snprintf_P(payload, remaining, JsonBase,
        channel.name,
        _config->mqtt.deviceId, channel.slug);

    if (ret > 0 && static_cast<size_t>(ret) < remaining)
    {
        offset += static_cast<size_t>(ret);
        remaining -= static_cast<size_t>(ret);
    }
    else
    {
        remaining = 0;
    }

    if (channel.isBinary && remaining > 0)
    {
        ret = snprintf_P(payload + offset, remaining, JsonBinaryPayload);

        if (ret > 0 && static_cast<size_t>(ret) < remaining)
        {
            offset += static_cast<size_t>(ret);
            remaining -= static_cast<size_t>(ret);
        }
        else
        {
            remaining = 0;
        }
    }

    if (channel.deviceClass != nullptr && remaining > 0)
    {
        ret = snprintf_P(payload + offset, remaining, JsonDeviceClass,
            channel.deviceClass);

        if (ret > 0 && static_cast<size_t>(ret) < remaining)
        {
            offset += static_cast<size_t>(ret);
            remaining -= static_cast<size_t>(ret);
        }
        else
        {
            remaining = 0;
        }
    }

    if (channel.unit != nullptr && remaining > 0)
    {
        ret = snprintf_P(payload + offset, remaining, JsonUnit,
            channel.unit);

        if (ret > 0 && static_cast<size_t>(ret) < remaining)
        {
            offset += static_cast<size_t>(ret);
            remaining -= static_cast<size_t>(ret);
        }
        else
        {
            remaining = 0;
        }
    }

    if (remaining > 0)
    {
        snprintf_P(payload + offset, remaining, JsonFooter,
            _config->mqtt.deviceId, channel.slug,
            _config->mqtt.deviceId);
    }

    client->publish(topic, payload, MqttQoS::AtMostOnce, true);
}
#endif