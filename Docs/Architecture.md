# SmartFuseBox – Architecture Overview

## 1. Project Purpose

SmartFuseBox is a modular, Arduino-powered **12V power distribution and control system** designed for marine and off-grid applications. It provides fused relay switching, onboard sensor monitoring, wireless connectivity (WiFi/BLE/MQTT), and a serial command interface to an external control panel.

---

## 2. Physical Units

The system spans two physical hardware units that communicate over a serial link.

| Unit | Hardware | Role |
|---|---|---|
| **Smart Fuse Box (SFB)** | Arduino Uno R4 (or ESP32) | Relay control, sensor handling, WiFi, BLE, MQTT, SD logging |
| **Boat Control Panel (BCP)** | Arduino Mega + Nextion display | User interface, immobiliser logic, time sync, config management |

The two units communicate via a 7-wire harness (12V in/out, GND, 5V, TX, RX, spare). The control panel must return a valid 12V feed to enable the fuse box, acting as a basic immobiliser.

---

## 3. High-Level Component Map

```
┌───────────────────────────────────────────────────────────────────┐
│                        SmartFuseBoxApp                            │
│                                                                   │
│  ┌───────────────┐  ┌────────────────┐  ┌──────────────────────┐  │
│  │ Serial Link   │  │  Message Bus   │  │   Warning Manager    │  │
│  │ (BroadcastMgr)│  │  (pub/sub)     │  │   (32-bit bitmap)    │  │
│  └───────────────┘  └────────────────┘  └──────────────────────┘  │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐   │
│  │                    Command Handlers (Serial)               │   │
│  │  SystemCmdHandler  RelayCmdHandler  SoundCmdHandler        │   │
│  │  SensorCmdHandler  ConfigCmdHandler  AckCmdHandler         │   │
│  └────────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐   │
│  │   Relay      │  │   Sensor     │  │   Config System        │   │
│  │   Controller │  │   Manager    │  │   (EEPROM + SD)        │   │
│  └──────────────┘  └──────────────┘  └────────────────────────┘   │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐   │
│  │   WiFi       │  │  Bluetooth   │  │   MQTT                 │   │
│  │   Controller │  │  Controller  │  │   Controller           │   │
│  │   + HTTP API │  │  (BLE)       │  │   (Home Assistant)     │   │
│  └──────────────┘  └──────────────┘  └────────────────────────┘   │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐   │
│  │  SD Card     │  │  LED Matrix  │  │  Date/Time Manager     │   │
│  │  Logger      │  │  Manager     │  │  (RTC + millis())      │   │
│  └──────────────┘  └──────────────┘  └────────────────────────┘   │
└───────────────────────────────────────────────────────────────────┘
```

---

## 4. Entry Point and Application Lifecycle

### `SmartFuseBox.ino`
The sketch file wires everything together. It owns the two `SerialCommandManager` instances (selecting which hardware serial ports and baud rates to use) and constructs `SmartFuseBoxApp`. Project-specific sensors are constructed using accessors from the app object.

```
setup()  →  initializeSerial  →  app.setup(sensors, remoteSensors)
loop()   →  app.loop()
```

### `SmartFuseBoxApp`
Central application class that owns all subsystems. On `setup()` it:
1. Initialises the `SensorManager` with all local and remote sensor handlers.
2. Configures WiFi and Bluetooth from the stored `Config`.
3. Registers all command handlers with each `SerialCommandManager`.
4. Registers all network handlers with the `WifiController`.
5. Registers all MQTT handlers (if `MQTT_SUPPORT` is defined).
6. Starts an LED startup animation (if `LED_MANAGER` is defined).

On `loop()` it delegates to each subsystem in turn.

---

## 5. Serial Communication Layer

All serial communication uses the `SerialCommandManager` library. Commands are newline-terminated with the following format:

```
{COMMAND}:{key=value};{key=value}\n
```

Two serial connections are maintained:

| Port | Baud | Purpose |
|---|---|---|
| `Serial` (COMPUTER_SERIAL) | 115200 | USB debug / computer connection |
| `Serial1` (LINK_SERIAL) | 19200 | Link to Boat Control Panel |

### `BroadcastManager`
Wraps both serial managers and provides a single `sendCommand()` / `sendDebug()` / `sendError()` interface that can target both ports simultaneously or link-only.

### Command Handlers
Each handler inherits `SharedBaseCommandHandler` and declares the command tokens it owns.

| Handler | Command Prefix | Responsibility |
|---|---|---|
| `SystemCommandHandler` | `F0`–`F11` | Heartbeat, memory/CPU diagnostics, date/time, SD status, uptime |
| `RelayCommandHandler` | `R0`–`R4` | Turn relays on/off, retrieve relay states |
| `SoundCommandHandler` | `H0`–`H12` | COLREGS sound signal playback |
| `SensorCommandHandler` | `S*` | Sensor data reporting and querying |
| `ConfigCommandHandler` | `C0`–`C31` | Full configuration management |
| `AckCommandHandler` | `ACK` | Processes acknowledgement responses |
| `WarningCommandHandler` | `W*` | Warning state propagation |
| `InterceptDebugHandler` | – | Debug message interception |

---

## 6. Relay Subsystem

### `RelayController`
Manages up to 8 relay outputs. Relay pins are LOW-active (ON = LOW, OFF = HIGH).

Key behaviours:
- **Linked relays**: configured pairs that mirror each other's state.
- **Reserved relay**: one relay index can be reserved for the sound (horn) system and is excluded from bulk on/off commands.
- **State change notifications**: publishes a `RelayStatusChanged` bitmask event on the `MessageBus` when any relay changes state.

---

## 7. Sensor Subsystem

### Base Classes

| Class | Role |
|---|---|
| `BaseSensor` | Extends `BaseSensorHandler` (external `SensorManager` library); optionally extends `JsonVisitor` for status serialisation |
| `RemoteSensor` | Receives updates from the serial link or MQTT; stores up to 5 key/value parameters |

All sensors declare a `SensorIdList` ID, a command ID string, and (when `MQTT_SUPPORT` is defined) expose named MQTT channels with device class, unit, and binary/numeric type metadata.

### Included Sensor Handlers

| Sensor | Type | Measurements |
|---|---|---|
| `WaterSensorHandler` | Local (analog) | Raw water level + rolling average queue |
| `Dht11SensorHandler` | Local (digital) | Temperature (°C) + Humidity (%) |
| `LightSensorHandler` | Local (digital + analog) | Day/night state |
| `SystemSensorHandler` | Local (virtual) | Free RAM, CPU usage, BT/WiFi state, SD log file size, active warning count |
| `RemoteSensor` (GPS) | Remote | Latitude, Longitude (extensible to Altitude, Speed, Course) |

### `SensorManager` (external library)
Calls `update()` on each registered sensor at its own requested interval (returned in milliseconds from `update()`). This keeps all sensor polling non-blocking.

### `SensorController`
Provides indexed access to all `BaseSensor` instances for the MQTT and network layers.

---

## 8. Connectivity

### 8.1 WiFi (`WifiController` + `WifiServer`)

Available on boards that define `WIFI_SUPPORT` (e.g. Arduino Uno R4, ESP32).

- Operates in **AP mode** (device creates its own network) or **Client mode** (joins existing network).
- `WifiServer` runs an HTTP server supporting up to 2 concurrent clients.
- REST API routes are registered via `INetworkCommandHandler` implementations:

| Network Handler | Route | Exposes |
|---|---|---|
| `SystemNetworkHandler` | `/api/system/{cmd}` | System commands (F0-only via WiFi) |
| `RelayNetworkHandler` | `/api/relay/{cmd}` | Relay state and control |
| `SoundNetworkHandler` | `/api/sound/{cmd}` | Sound signal commands |
| `ConfigNetworkHandler` | `/api/config/{cmd}` | Configuration read/write |
| `SensorNetworkHandler` | `/api/sensor/{cmd}` | Sensor data |
| `WarningNetworkHandler` | `/api/warning/{cmd}` | Warning state |

### 8.2 Bluetooth BLE (`BluetoothController` + `BluetoothManager`)

Available on boards that define `ARDUINO_UNO_R4`. Uses the `ArduinoBLE` library.

`BluetoothManager` coordinates the BLE stack and delegates domain logic to services:

| BLE Service | Exposes |
|---|---|
| `BluetoothSystemService` | Heartbeat, system initialised, free memory, CPU usage (notify characteristics) |
| `BluetoothRelayService` | Relay state array (read/notify) + relay set command (write) |
| `BluetoothSensorService` | Sensor data streaming |

### 8.3 MQTT (`MQTTController` + `MQTTClient`)

Available when both `WIFI_SUPPORT` and `MQTT_SUPPORT` are defined.

- `MQTTClient` is a pure C++ MQTT 3.1.1 implementation over `WiFiClient` (no external MQTT library dependency).
- `MQTTController` manages connection lifecycle: retry with exponential back-off, reconnect count, uptime tracking.
- Supports Home Assistant auto-discovery (`useHomeAssistantDiscovery` config flag).

| MQTT Handler | Responsibility |
|---|---|
| `MQTTSensorHandler` | Publishes all sensor channel states; handles HA discovery payloads |
| `MQTTRelayHandler` | Publishes relay states; subscribes to relay command topics |
| `MQTTSystemHandler` | Publishes system metrics |
| `MQTTConfigCommandHandler` | Handles configuration updates via MQTT |

---

## 9. Configuration System

| Component | Role |
|---|---|
| `Config` struct | Packed POD stored in EEPROM; versioned with checksum validation |
| `ConfigManager` | Static EEPROM read/write; `resetToDefaults()`; checksum calculation |
| `ConfigController` | Domain façade exposing typed setters (e.g. `setWifiSsid`, `setLedColor`) |
| `ConfigSyncManager` | Periodically requests a C1 (Get Settings) from the Control Panel every 5 minutes; retries every 10 seconds on failure |
| `SDCardConfigLoader` | Optional: loads `config.txt` from SD card on boot or card insertion |

Notable configuration fields include vessel name, relay names (short 5-char + long 20-char), home page button mapping, vessel type (Motor/Sail/Fishing/Yacht), MMSI, call sign, home port, timezone offset, LED colours/brightness, WiFi credentials, MQTT credentials, relay default states, and linked relay pairs.

---

## 10. Event System – `MessageBus`

`MessageBus` is a compile-time typed publish/subscribe bus. Components subscribe to strongly-typed event structs; publishers call `messageBus->publish<EventType>(args)`.

Key events:

| Event | Payload | Published by |
|---|---|---|
| `TemperatureUpdated` | `float newTemp` | `Dht11SensorHandler` |
| `HumidityUpdated` | `uint8_t newHumidity` | `Dht11SensorHandler` |
| `LightSensorUpdated` | `bool isDayTime` | `LightSensorHandler` |
| `WaterLevelUpdated` | `uint16_t level, uint16_t avg` | `WaterSensorHandler` |
| `RelayStatusChanged` | `uint8_t changedMask` | `RelayController` |
| `WifiConnectionStateChanged` | `WifiConnectionState` | `WifiServer` |
| `WifiSignalStrengthChanged` | `uint16_t strength` | `WifiServer` |
| `GpsLocationUpdated` | `double lat, double lon` | `RemoteSensor` |
| `SystemMetricsUpdated` | – | `SystemSensorHandler` |
| `MqttConnected` / `MqttDisconnected` | – | `MQTTController` |
| `MqttMessageReceived` | `topic, payload` | `MQTTController` |
| `WarningChanged` | `uint32_t mask` | `WarningManager` |

---

## 11. Warning System – `WarningManager`

Tracks up to 32 simultaneous warnings using a `uint32_t` bitmask, split into local (raised on this device) and remote (received from the Control Panel).

| Bit Range | Category |
|---|---|
| 0–19 | System warnings (connection lost, WiFi/BT init failed, SD card issues, low battery, etc.) |
| 20–31 | Sensor warnings (temperature sensor failure, GPS failure, compass failure, etc.) |

Built-in heartbeat monitoring automatically raises/clears the `ConnectionLost` warning based on `F0` ACK timing (timeout: 3 seconds).

---

## 12. Sound System – `SoundController`

Non-blocking COLREGS-compliant sound signal playback via a relay-controlled horn. Uses a state machine (`Idle → StartDelay → BlastOn → BlastGap → WaitingRepeat`).

Supported signal types:

| Signal | COLREGS Reference |
|---|---|
| SOS | Distress (Morse) |
| Fog | Rule 35 (repeats every 2 minutes) |
| MoveStarboard / MovePort / MoveAstern / MoveDanger | Rule 34 manoeuvring signals |
| OvertakeStarboard / OvertakePort / OvertakeConsent / OvertakeDanger | Rule 34 overtaking signals |
| Test | Non-COLREGS test blast |

---

## 13. SD Card and Logging

| Component | Role |
|---|---|
| `MicroSdDriver` | Singleton resource manager for the SD card (SdFat library); handles card presence detection, settling delays, and up to 3 simultaneous file handles |
| `SdCardLogger` | Non-blocking sensor data logger; buffers up to 64 records in a circular buffer; writes a maximum of 5 records per loop iteration; creates date-named CSV files (`YYYYMMDD.csv`) with automatic midnight rotation |

`SdCardLogger` subscribes to `MessageBus` sensor events and captures a `SensorSnapshot` (temperature, humidity, water level, GPS, warnings, etc.) on each update.

---

## 14. LED Matrix – `LedMatrixManager`

Used on Arduino Uno R4 boards with `LED_MANAGER` defined. Drives the 8×12 built-in LED matrix.

Matrix layout (conceptual):

```
Row 0     : Top border / relay states
Col 0     : Left border / system indicator
Cols 3–4  : Temperature display (rows 2–7)
Cols 10–11: Warning / GPS indicators (rows 4–7)
```

Reacts to `MessageBus` events to update:
- Relay on/off states (top row)
- WiFi connection state and RSSI signal bars
- Temperature and humidity readings
- Warning indicators (GPS, system, warnings)

Supports startup and shutdown animation sequences.

---

## 15. Date/Time – `DateTimeManager`

Static class providing system-wide time management.

- Tracks Unix timestamp (seconds) plus a millisecond offset derived from `millis()`.
- On boot, attempts to read time from a DS1302 hardware RTC (Control Panel builds only).
- Time is continuously synchronised via the `t=` parameter in the `F0` heartbeat command from the Control Panel — no separate `F6` command is needed during normal operation.
- Exposes `setDateTimeISO()` and `setDateTime(unixTimestamp)`.

---

## 16. Feature Flags (`Local.h`)

All major features are controlled by compile-time defines in `Local.h` (consumer-editable):

| Define | Effect |
|---|---|
| `ARDUINO_UNO_R4` | Enables WiFi and Bluetooth support |
| `FUSE_BOX_CONTROLLER` | Includes relay/sensor controller code; omit to use components standalone |
| `WIFI_SUPPORT` | Includes `WifiController`, `WifiServer`, and network command handlers |
| `MQTT_SUPPORT` | Includes `MQTTController`, `MQTTClient`, and all MQTT handlers (requires `WIFI_SUPPORT`) |
| `LED_MANAGER` | Includes `LedMatrixManager` (requires `ARDUINO_UNO_R4`) |
| `CARD_CONFIG_LOADER` | Includes `SDCardConfigLoader` for SD-based config loading |

---

## 17. Key Design Patterns

| Pattern | Where Used |
|---|---|
| **Facade / Owned-component** | `SmartFuseBoxApp` owns all subsystems and exposes accessor methods to the sketch |
| **Publish / Subscribe** | `MessageBus` decouples producers (sensors, relay, WiFi) from consumers (LED matrix, MQTT, logger) |
| **Command Handler** | Serial, HTTP, and MQTT commands each routed through dedicated handler classes |
| **Strategy / Visitor** | `JsonVisitor` / `NetworkJsonVisitor` decouple JSON serialisation from domain objects |
| **Sensor Polling Abstraction** | External `SensorManager` library calls `update()` on each sensor at its own interval, avoiding `delay()` |
| **Mixin Logging** | `LoggingSupport`, `SingleLoggerSupport`, `BroadcastLoggerSupport` add logging to any class via multiple inheritance |
| **Compile-time Feature Selection** | `#define` flags in `Local.h` prevent unused subsystems from consuming flash/RAM on resource-constrained boards |

