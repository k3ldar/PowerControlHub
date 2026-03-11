# SmartFuseBox – Message Bus

The `MessageBus` class provides a lightweight publish/subscribe event system used to decouple components within the firmware. Subscribers register a typed callback; publishers fire the event with matching arguments. No dynamic allocation occurs at publish time — all subscriber vectors are function-local statics.

---

## Usage

**Subscribe**
```cpp
_messageBus->subscribe<LightSensorUpdated>(
    [this](bool isDayTime, uint16_t lightLevel, uint16_t averageLightLevel)
    {
        // handle event
    }
);
```

**Publish**
```cpp
_messageBus->publish<LightSensorUpdated>(true, 620, 587);
```

The template parameter must be one of the event structs defined in `MessageBus.h`. The argument types must exactly match the `Callback` signature defined in that struct.

---

## Event Reference

### System & Config

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `WarningChanged` | `void(uint32_t mask)` | `WarningManager` | Fired when the active warning bitmask changes. `mask` is the new full bitmask. |
| `SystemMetricsUpdated` | `void()` | `SystemSensorHandler` | Fired on each system sensor poll. Triggers `MQTTSensorHandler` to republish all sensor states. |

---

### Sensors

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `TemperatureUpdated` | `void(float newTemp)` | `Dht11SensorHandler` | Fired after each successful DHT11 read with the new temperature in °C. |
| `HumidityUpdated` | `void(uint8_t newHumidity)` | `Dht11SensorHandler` | Fired after each successful DHT11 read with the new relative humidity (%). |
| `LightSensorUpdated` | `void(bool isDayTime, uint16_t lightLevel, uint16_t averageLightLevel)` | `LightSensorHandler` | Fired on every light sensor poll. `isDayTime` reflects the debounce-confirmed state. `lightLevel` is the raw ADC reading (0 – 1023). `averageLightLevel` is the rolling queue average. |
| `WaterLevelUpdated` | `void(uint16_t newWaterLevel, uint16_t averageWaterLevel)` | `WaterSensorHandler` | Fired on every water sensor poll with the instantaneous and rolling average ADC readings. |

---

### Relay

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `RelayStatusChanged` | `void(uint8_t status)` | `RelayController` | Fired when one or more relays change state. `status` is a bitmask of the relays that actually changed (bit 0 = relay 0, bit 7 = relay 7). |

---

### GPS

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `GpsLocationUpdated` | `void(double latitude, double longitude)` | `GpsSensorHandler` | Fired on each new GPS fix with the current latitude and longitude. |
| `GpsAltitudeUpdated` | `void(double altitude)` | `GpsSensorHandler` | Fired on each new GPS fix with the current altitude in metres. |
| `GpsSpeedUpdated` | `void(double speedKmh, double course)` | `GpsSensorHandler` | Fired on each new GPS fix with the speed in km/h and course in degrees. |

---

### WiFi

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `WifiConnectionStateChanged` | `void(WifiConnectionState status)` | `WifiController` | Fired when the WiFi connection state changes (connecting, connected, disconnected, etc.). |
| `WifiSignalStrengthChanged` | `void(uint16_t strength)` | `WifiController` | Fired periodically with the current RSSI signal strength. |
| `WifiServerProcessingRequestChanged` | `void(const char* method, const char* path, const char* query, bool isProcessing)` | `WifiServer` | Fired when the HTTP server starts or finishes processing a request. |

---

### MQTT

Available when `MQTT_SUPPORT` is defined.

| Event | Callback Signature | Published by | Description |
|---|---|---|---|
| `MqttConnected` | `void()` | `MQTTController` | Fired when the MQTT broker connection is established. Triggers topic subscription and state republish. |
| `MqttDisconnected` | `void()` | `MQTTController` | Fired when the MQTT broker connection is lost. |
| `MqttMessageReceived` | `void(const char* topic, const char* payload)` | `MQTTController` | Fired when an inbound MQTT message arrives. Routed to the appropriate handler by topic. |
