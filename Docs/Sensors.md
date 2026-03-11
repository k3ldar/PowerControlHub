# SmartFuseBox – Sensor Support

This document describes the sensors supported by SmartFuseBox, their hardware requirements, configuration, serial command IDs, and MQTT integration.

---

## Overview

SmartFuseBox supports two categories of sensors:

| Type | Description |
|---|---|
| **Local** | Sensors physically connected to the SmartFuseBox controller board and polled directly by the main loop. |
| **Remote** | Sensors on a companion device (e.g., Boat Control Panel) that push readings to the fuse box via serial commands. |

All sensors inherit from `BaseSensor` and are registered with `SensorController`. When `MQTT_SUPPORT` is defined, each sensor exposes one or more channels that are automatically published to an MQTT broker and discovered by Home Assistant.

---

## 1. Local Sensors

### 1.1 DHT11 – Temperature & Humidity

| Property | Value |
|---|---|
| **Class** | `Dht11SensorHandler` |
| **Sensor ID** | `SensorIdList::Dht11Sensor` (`0x1`) |
| **Default Pin** | `Dht11SensorPin` (`D9`) |
| **Poll Interval** | 2 500 ms |
| **Library** | `dht11` |

**Serial Commands**

| Command | ID | Description |
|---|---|---|
| Temperature | `S0` | Current temperature in °C |
| Humidity | `S1` | Current relative humidity (%) |

**MQTT Channels**

| Channel | Slug | Device Class | Unit | Binary |
|---|---|---|---|---|
| Temperature | `temperature` | `temperature` | °C | No |
| Humidity | `humidity` | `humidity` | % | No |

**Messages Published (MessageBus)**

- `TemperatureUpdated` – fired after each successful read with the new temperature in °C.
- `HumidityUpdated` – fired after each successful read with the new humidity percentage.

**Warnings**

- `TemperatureSensorFailure` – raised when the DHT11 returns a read error; cleared automatically on the next successful read.

**JSON Status Fragment**

```json
{ "temperature": 22.5, "humidity": 60.0 }
```

---

### 1.2 Light Sensor (LDR)

| Property | Value |
|---|---|
| **Class** | `LightSensorHandler` |
| **Sensor ID** | `SensorIdList::LightSensor` (`0x2`) |
| **Digital Pin** | `LightSensorPin` (`D3`) — kept for `pinMode` initialisation |
| **Analog Pin** | `LightSensorAnalogPin` (`A1`) — primary reading source |
| **Poll Interval** | 30 000 ms |
| **Averaging** | Rolling queue of 10 readings (`LightQueueCapacity`) |
| **Debounce** | 3 consecutive agreeing readings required (`DaytimeConfirmReadings`) |

#### Detection Logic

Each poll reads the raw ADC value from the analog pin (0 – 1023) and enqueues it into a rolling `Queue<uint16_t>`. The queue automatically discards the oldest reading when full, giving a sliding average over the last 10 samples (≈ 5 minutes at the default poll interval).

The averaged value is compared against the configurable `daytimeThreshold` (default 512, stored in `Config.lightSensor.daytimeThreshold`):

- `average > threshold` → **daytime** candidate
- `average ≤ threshold` → **night** candidate

A candidate must be consistent for **3 consecutive polls** before `_isDaytime` changes state. This prevents rapid relay switching caused by transient light fluctuations at dusk and dawn.

#### Night Relay

If `Config.lightSensor.nightRelayIndex` is set to a valid relay index (0 – 7), that relay is automatically switched:

- **ON** when night is confirmed
- **OFF** when daytime is confirmed
- Driven to the correct initial state during `initialize()`

Set to `0xFF` (default) to disable automatic relay control.

#### Configuration (`Config.lightSensor`)

| Field | Type | Default | Description |
|---|---|---|---|
| `nightRelayIndex` | `uint8_t` | `0xFF` | Relay to drive on night/day transitions. `0xFF` = none |
| `daytimeThreshold` | `uint16_t` | `512` | ADC threshold (0 – 1023). Readings above this are treated as daytime |

Configure via serial commands `C32` / `C33`, or through the **Relay Settings** page on the Boat Control Panel (night relay only; threshold via serial).

**Serial Commands**

| Command | ID | Description |
|---|---|---|
| Light Sensor | `S9` | `1` = daytime, `0` = night |

**Config Commands**

| Command | ID | Format | Description |
|---|---|---|---|
| Night Relay | `C32` | `v=<0-7\|255>` | Set night relay index; `255` = none |
| Daytime Threshold | `C33` | `v=<0-1023>` | Set ADC daytime threshold |

**MQTT Channels**

| Index | Channel | Slug | Device Class | Unit | Binary |
|---|---|---|---|---|---|
| 0 | Daylight | `light` | `light` | — | Yes (`ON` / `OFF`) |
| 1 | Light Level | `light_level` | — | — | No |
| 2 | Avg Light Level | `avg_light_level` | — | — | No |

**Messages Published (MessageBus)**

- `LightSensorUpdated(bool isDayTime, uint16_t lightLevel, uint16_t averageLightLevel)` – fired on every poll with the confirmed day/night state, the raw ADC reading, and the current rolling average.

**JSON Status Fragment**

```json
{ "isDaytime": true, "lightLevel": 620, "avgLightLevel": 587 }
```

---

### 1.3 Water Level Sensor (Analog)

| Property | Value |
|---|---|
| **Class** | `WaterSensorHandler` |
| **Sensor ID** | `SensorIdList::WaterSensor` (`0x0`) |
| **Analog Pin** | `WaterSensorPin` (`A0`) |
| **Active Pin** | `WaterSensorActivePin` (`D8`) |
| **Poll Interval** | 5 000 ms (+ 10 ms stabilisation delay) |
| **Averaging** | Rolling queue of 15 readings |

The active pin is pulsed `HIGH` before each reading and pulled `LOW` immediately after to reduce electrolytic corrosion on the sensor probes.

**Serial Commands**

| Command | ID | Parameters | Description |
|---|---|---|---|
| Water Level | `S6` | `avg`, `v` | Rolling average and instantaneous ADC value |

**MQTT Channels**

| Channel | Slug | Device Class | Unit | Binary |
|---|---|---|---|---|
| Water Level | `water_level` | — | — | No |

The published MQTT value is the 15-reading rolling average.

**Messages Published (MessageBus)**

- `WaterLevelUpdated` – fired on every poll with the instantaneous reading and rolling average.

**JSON Status Fragment**

```json
{ "level": 312, "average": 305 }
```

---

### 1.4 System Diagnostics Sensor

| Property | Value |
|---|---|
| **Class** | `SystemSensorHandler` |
| **Sensor ID** | `SensorIdList::SystemSensor` (`0x4`) |
| **Poll Interval** | 2 500 ms |

This sensor does not read any hardware pin. It exposes internal device health metrics.

**MQTT Channels (7)**

| Index | Channel | Slug | Device Class | Unit | Binary |
|---|---|---|---|---|---|
| 0 | Sys Free Memory | `free_memory` | — | bytes | No |
| 1 | Sys CPU Usage | `cpu_usage` | — | % | No |
| 2 | Sys Bluetooth | `bluetooth` | `connectivity` | — | Yes (`ON` / `OFF`) |
| 3 | Sys WiFi | `wifi` | `connectivity` | — | Yes (`ON` / `OFF`) |
| 4 | Sys SD Log Size | `sd_log_size` | — | MB | No |
| 5 | Sys Warnings | `warning_count` | — | — | No |
| 6 | Sys Uptime | `uptime` | — | — | No |

**Messages Published (MessageBus)**

- `SystemMetricsUpdated` – published on each poll interval; triggers `MQTTSensorHandler` to republish all sensor states.

**JSON Status Fragment**

```json
{ "freeMemory": 2048, "cpuUsage": 12 }
```

---

## 2. Remote Sensors

Remote sensors receive data pushed from a companion device over the serial link. The data arrives as a serial command and is processed by `SensorCommandHandler`, which forwards the key-value parameters to the matching `RemoteSensor` instance.

Each `RemoteSensor` supports up to `MaxRemoteParams` (5) key-value parameters stored in RAM and served to the MQTT pipeline on demand. The internal poll interval is 60 000 ms, but updates occur whenever the matching serial command arrives rather than on a fixed timer.

### 2.1 GPS Location (Lat / Lon)

| Property | Value |
|---|---|
| **Instance** | `gpsLatLonSensor` |
| **Class** | `RemoteSensor` |
| **Sensor ID** | `SensorIdList::GpsSensor` (`0x3`) |
| **Command ID** | `S10` (`SensorGpsLatLong`) |
| **Parameter Count** | 2 (`lat`, `lon`) |

**MQTT Channels**

| Channel | Slug | Device Class | Unit | Binary |
|---|---|---|---|---|
| Latitude | `latitude` | — | ° | No |
| Longitude | `longitude` | — | ° | No |

---

## 3. Optional Sensors

The sensors below are provided as ready-to-use handlers but are not wired into `SmartFuseBox.ino` by default. Instantiate them in the sketch and add them to the `localSensors[]` array to enable them.

### 3.1 GPS – Full Local Handler (GY-GPS6MV2)

| Property | Value |
|---|---|
| **Class** | `GpsSensorHandler` |
| **Sensor ID** | `SensorIdList::GpsSensor` (`0x3`) |
| **Library** | `TinyGPS++` |
| **Poll Interval** | 10 ms (NMEA streaming) |
| **Time Sync** | Every 5 minutes from GPS UTC |
| **Stale Fix Timeout** | 30 s → raises `GpsFailure` warning |

The handler reads NMEA sentences from any `Stream*` (hardware UART, `SoftwareSerial`, etc.) and computes speed and course from the delta between consecutive fixes using the Haversine formula.

**Serial Commands**

| Command | ID | Description |
|---|---|---|
| GPS Lat/Lon | `S10` | Latitude and longitude (6 decimal places) |
| GPS Altitude | `S11` | Altitude in metres |
| GPS Speed | `S12` | Speed (km/h) and course (°) |
| GPS Satellites | `S13` | Number of satellites in view |
| Direction | `S3` | 16-point compass string (e.g. `NNE`) |
| Bearing | `S2` | Course in degrees |
| GPS Distance | `S14` | Cumulative distance travelled in km |

**Messages Published (MessageBus)** *(requires `MESSAGE_BUS` define)*

- `GpsLocationUpdated` – latitude and longitude on each new fix.
- `GpsAltitudeUpdated` – altitude on each new fix.
- `GpsSpeedUpdated` – speed and course on each new fix.

**Warnings**

- `GpsFailure` – raised when no valid fix has been received for 30 seconds; cleared automatically on the next valid fix.

**JSON Status Fragment**

```json
{
  "gps": {
    "lat": 51.500000,
    "lon": -0.120000,
    "alt": 15.00,
    "speed": 4.50,
    "course": 270.00,
    "sats": 8,
    "valid": true
  }
}
```

---

## 4. Sensor Infrastructure

### 4.1 Class Hierarchy

```
BaseSensorHandler  (SensorManager library)
└── BaseSensor
    ├── Dht11SensorHandler      (local – temperature & humidity)
    ├── LightSensorHandler      (local – daylight detection)
    ├── WaterSensorHandler      (local – analog water level)
    ├── SystemSensorHandler     (local – device diagnostics)
    ├── GpsSensorHandler        (local – full GPS, optional)
    └── RemoteSensor            (remote – generic serial-pushed sensor)
```

### 4.2 Adding a New Local Sensor

A local sensor is physically connected to the SmartFuseBox board and polled directly by the main loop.

**Step 1 – Add identifiers to `SystemDefinitions.h`**

Add a command ID constant and a unique `SensorIdList` entry:

```cpp
// New command ID – choose the next available Sxx number
constexpr char SensorMyThing[] = "S15";

// New SensorIdList entry
enum class SensorIdList : uint8_t
{
    // ...existing entries...
    MyThingSensor = 0x5
};
```

**Step 2 – Create the sensor handler class**

Inherit from `BaseSensor` and implement all required methods. Place the file alongside the other sensor handlers in `Shared/Sensors/`.

```cpp
#pragma once
#include "BaseSensor.h"

class MyThingSensorHandler : public BaseSensor
{
private:
    const uint8_t _sensorPin;
    int _lastValue;

protected:
    void initialize() override
    {
        pinMode(_sensorPin, INPUT);
    }

    unsigned long update() override
    {
        _lastValue = analogRead(_sensorPin);

        StringKeyValue params;
        strncpy(params.key, ValueParamName, sizeof(params.key));
        snprintf(params.value, sizeof(params.value), "%d", _lastValue);
        sendCommand(SensorMyThing, &params, 1);

        return 5000; // poll every 5 000 ms
    }

public:
    MyThingSensorHandler(BroadcastManager* broadcastManager, uint8_t sensorPin)
        : BroadcastLoggerSupport(broadcastManager), _sensorPin(sensorPin), _lastValue(0)
    {
    }

    void formatStatusJson(char* buffer, size_t size) override
    {
        snprintf(buffer, size, "\"myThing\":%d", _lastValue);
    }

    SensorIdList getSensorId() const override { return SensorIdList::MyThingSensor; }
    SensorType getSensorType() const override { return SensorType::Local; }
    const char* getSensorCommandId() const override { return SensorMyThing; }
    const char* getSensorName() const override { return "myThing"; }

#if defined(MQTT_SUPPORT)
    uint8_t getMqttChannelCount() const override { return 1; }

    MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
    {
        (void)channelIndex;
        return { "My Thing", "my_thing", nullptr, "units", false };
    }

    void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
    {
        (void)channelIndex;
        snprintf(buffer, size, "%d", _lastValue);
    }
#endif
};
```

**Step 3 – Register the sensor in `SmartFuseBox.ino`**

Instantiate the handler using the app accessors, then add it to `localSensors[]`:

```cpp
#include "MyThingSensorHandler.h"

MyThingSensorHandler myThingSensorHandler(app.broadcastManager(), MyThingPin);

BaseSensorHandler* localSensors[] = {
    &waterSensorHandler,
    &dht11SensorHandler,
    &lightSensorHandler,
    &systemSensorHandler,
    &gpsLatLonSensor,
    &myThingSensorHandler   // <-- add here
};
```

**Step 4 – Add a pin constant to `Local.h`** *(if needed)*

```cpp
constexpr uint8_t MyThingPin = A2;
```

---

### 4.3 Adding a New Remote Sensor

A remote sensor is driven by a companion device (e.g., Boat Control Panel). The companion sends a serial command containing the sensor values; `SensorCommandHandler` receives it and forwards the key-value parameters to the matching `RemoteSensor` instance.

**Step 1 – Add identifiers to `SystemDefinitions.h`**

```cpp
constexpr char SensorMyRemoteThing[] = "S16";

enum class SensorIdList : uint8_t
{
    // ...existing entries...
    MyRemoteSensor = 0x6
};
```

**Step 2 – Register the command in `SensorCommandHandler.cpp`**

Add the new constant to `supportedCommands()` so the serial manager routes incoming commands to the handler, and add a case in `handleCommand()` to update internal state and support query responses (zero-parameter calls):

```cpp
// In supportedCommands():
static const char* cmds[] = {
    // ...existing entries...
    SensorMyRemoteThing
};

// In handleCommand() – zero-param query block:
else if (strcmp(command, SensorMyRemoteThing) == 0)
{
    snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastMyRemoteValue);
    sender->sendCommand(SensorMyRemoteThing, buffer);
    sendAckOk(sender, command);
    return true;
}

// In handleCommand() – data update block:
else if (strcmp(command, SensorMyRemoteThing) == 0)
{
    _lastMyRemoteValue = atoi(params[0].value);
}
```

Add the corresponding private field and getter/setter pair to `SensorCommandHandler.h` if the value needs to be accessible to other subsystems.

**Step 3 – Declare the remote sensor in `SmartFuseBox.ino`**

```cpp
#if defined(MQTT_SUPPORT)
MqttSensorChannel myRemoteMqttChannels[] = {
    { "My Remote Thing", "my_remote_thing", nullptr, "units", false }
};
RemoteSensor myRemoteSensor(
    SensorIdList::MyRemoteSensor,
    "MyRemote",
    SensorMyRemoteThing,
    "MyRemote",
    myRemoteMqttChannels,
    1   // parameter count
);
#else
RemoteSensor myRemoteSensor(SensorIdList::MyRemoteSensor, "MyRemote", SensorMyRemoteThing, 1);
#endif
```

**Step 4 – Add to `remoteSensors[]` and `localSensors[]`**

The sensor must appear in **both** arrays:

- `remoteSensors[]` – registers it with `SensorCommandHandler` so incoming serial commands update its stored values.
- `localSensors[]` – registers it with `SensorManager` and `SensorController` so it participates in the polling loop and MQTT publishing pipeline.

```cpp
RemoteSensor* remoteSensors[] = {
    &gpsLatLonSensor,
    &myRemoteSensor   // <-- add here
};

BaseSensorHandler* localSensors[] = {
    &waterSensorHandler,
    &dht11SensorHandler,
    &lightSensorHandler,
    &systemSensorHandler,
    &gpsLatLonSensor,
    &myRemoteSensor   // <-- add here too
};
```

### 4.4 MQTT Discovery

When `MQTT_SUPPORT` is defined and `mqtt.useHomeAssistantDiscovery` is `true` in the stored configuration, `MQTTSensorHandler` publishes a Home Assistant MQTT discovery config message for every channel on connection. State updates are published:

- On MQTT reconnect (all channels).
- When a typed `MessageBus` event fires (e.g., `TemperatureUpdated`, `WaterLevelUpdated`).
- Every 2 500 ms when `SystemMetricsUpdated` is published by `SystemSensorHandler`.

Discovery topic format: `<prefix>/<entity_type>/<device_id>/<slug>/config`  
State topic format: `home/<device_id>/sensor/<slug>/state`

### 4.5 Serial Command IDs

All sensor serial command identifiers are defined in `SystemDefinitions.h`:

| Constant | ID | Description |
|---|---|---|
| `SensorTemperature` | `S0` | Temperature (°C) |
| `SensorHumidity` | `S1` | Relative humidity (%) |
| `SensorBearing` | `S2` | GPS bearing / course (°) |
| `SensorDirection` | `S3` | Compass direction string |
| `SensorSpeed` | `S4` | Speed (km/h) |
| `SensorCompassTemp` | `S5` | Compass module temperature |
| `SensorWaterLevel` | `S6` | Water level ADC value |
| `SensorWaterPumpActive` | `S7` | Water pump active state |
| `SensorHornActive` | `S8` | Horn active state |
| `SensorLightSensor` | `S9` | Light sensor (daytime flag) |
| `SensorGpsLatLong` | `S10` | GPS latitude and longitude |
| `SensorGpsAltitude` | `S11` | GPS altitude (m) |
| `SensorGpsSpeed` | `S12` | GPS speed and course |
| `SensorGpsSatellites` | `S13` | GPS satellite count |
| `SensorGpsDistance` | `S14` | Cumulative GPS distance (km) |
