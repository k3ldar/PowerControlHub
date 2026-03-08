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

#include "MQTTHandler.h"
#include "ConfigManager.h"
#include "SensorController.h"
#include <vector>

// Forward declaration
class SerialCommandManager;

struct MqttChannelMap
{
    BaseSensor* sensor;
    uint8_t channelIndex;
};

class MQTTSensorHandler : public MQTTHandler
{
private:
    Config* _config;
    SensorController* _sensorController;
    SerialCommandManager* _commandMgr;

    // Channel map (built during begin())
    std::vector<MqttChannelMap> _channelMap;

    // Channel indices for typed MessageBus events (-1 if not present)
    int8_t _tempChannelIdx;
    int8_t _humidityChannelIdx;
    int8_t _lightChannelIdx;
    int8_t _waterChannelIdx;

    // Discovery state
    bool _discoveryPending;
    uint8_t _discoveryIndex;
    unsigned long _lastDiscoveryPublish;

    static constexpr uint16_t MinPublishInterval = 200;

    void publishSensorState(uint8_t channelIndex);
    void publishDiscoveryConfig();
    void publishSensorDiscoveryConfig(uint8_t index);
    void onTemperatureUpdated(float temperature);
    void onHumidityUpdated(uint8_t humidity);
    void onLightSensorUpdated(bool isDaytime);
    void onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel);

public:
    MQTTSensorHandler(MQTTController* mqttController, MessageBus* messageBus, SensorController* sensorController, SerialCommandManager* commandMgr = nullptr);

    bool begin() override;
    void update() override;
    void end() override;

    void onMessage(const char* topic, const char* payload) override;

    bool subscribe() override;
    void unsubscribe() override;
};
