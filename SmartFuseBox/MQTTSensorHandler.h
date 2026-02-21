#pragma once
#include "Local.h"

#if defined(MQTT_SUPPORT)

#include "MQTTHandler.h"
#include "ConfigManager.h"
#include "SensorController.h"

// Forward declaration
class SerialCommandManager;

constexpr uint8_t MaxMqttSensorChannels = 8;

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
    MqttChannelMap _channelMap[MaxMqttSensorChannels];
    bool _dirtyChannels[MaxMqttSensorChannels];
    uint8_t _totalChannels;

    // Channel indices for typed MessageBus events (-1 if not present)
    int8_t _tempChannelIdx;
    int8_t _humidityChannelIdx;
    int8_t _lightChannelIdx;
    int8_t _waterChannelIdx;

    // Discovery state
    bool _discoveryPending;
    uint8_t _discoveryIndex;
    unsigned long _lastDiscoveryPublish;

    // Throttled state publishing
    unsigned long _lastStatePublish;
    static constexpr uint16_t MinPublishInterval = 200;
    static constexpr uint16_t MaxPublishInterval = 5000;

    // State publishing
    void publishSensorState(uint8_t channelIndex);
    void publishDirtySensorStates();
    void markChannelDirty(int8_t channelIndex);
    void markAllChannelsDirty();

    // Home Assistant Discovery
    void publishDiscoveryConfig();
    void publishSensorDiscoveryConfig(uint8_t index);

    // MessageBus event handlers
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

#endif