#pragma once
#include <functional>
#include <vector>
#include <stdint.h>
#include "SystemDefinitions.h"

struct WarningChanged {
    using Callback = std::function<void(uint32_t mask)>;
};

struct TemperatureUpdated {
    using Callback = std::function<void(float newTemp)>;
};

struct HumidityUpdated {
    using Callback = std::function<void(float newHumidity)>;
};

struct WifiConnectionStateChanged {
    using Callback = std::function<void(WifiConnectionState status)>;
};

struct WifiSignalStrengthChanged {
    using Callback = std::function<void(uint16_t strength)>;
};

struct RelayStatusChanged {
    using Callback = std::function<void(uint8_t status)>;
};

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