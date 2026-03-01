#pragma once
#include <functional>
#include <vector>
#include <stdint.h>

#include "Local.h"
#include "SystemDefinitions.h"

struct WarningChanged {
    using Callback = std::function<void(uint32_t mask)>;
};

struct TemperatureUpdated {
    using Callback = std::function<void(float newTemp)>;
};

struct HumidityUpdated {
    using Callback = std::function<void(uint8_t newHumidity)>;
};

struct LightSensorUpdated {
    using Callback = std::function<void(bool isDayTime)>;
};

struct WaterLevelUpdated {
    using Callback = std::function<void(uint16_t newWaterLevel, uint16_t averageWaterLevel)>;
};

#if defined(WIFI_SUPPORT)
struct WifiConnectionStateChanged {
    using Callback = std::function<void(WifiConnectionState status)>;
};

struct WifiSignalStrengthChanged {
    using Callback = std::function<void(uint16_t strength)>;
};

struct WifiServerProcessingRequestChanged {
    using Callback = std::function<void(const char* method, const char* path, const char* query, bool isProcessing)>;
};
#endif

struct RelayStatusChanged {
    using Callback = std::function<void(uint8_t status)>;
};

struct GpsLocationUpdated {
    using Callback = std::function<void(double latitude, double longitude)>;
};

struct GpsAltitudeUpdated {
    using Callback = std::function<void(double altitude)>;
};

struct GpsSpeedUpdated {
    using Callback = std::function<void(double speedKmh, double course)>;
};

struct SystemMetricsUpdated {
    using Callback = std::function<void()>;
};

#if defined(MQTT_SUPPORT)

struct MqttConnected {

    using Callback = std::function<void()>;

};

struct MqttDisconnected {

    using Callback = std::function<void()>;

};

struct MqttMessageReceived {

    using Callback = std::function<void(const char* topic, const char* payload)>;

};

#endif

class MessageBus {
private:
    template<typename Event>
    std::vector<typename Event::Callback>& getVector() {
        static std::vector<typename Event::Callback> vec;
        return vec;
    }
public:
    template<typename Event, typename Callback>
    void subscribe(Callback cb) {
        auto& vec = getVector<Event>();
        vec.push_back(cb);
    }

    template<typename Event, typename... Args>
    void publish(Args&&... args) {
        auto& vec = getVector<Event>();
        for (auto& cb : vec)
            cb(std::forward<Args>(args)...);
    }

};