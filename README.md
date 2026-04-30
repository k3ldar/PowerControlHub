# PowerControlHub  
PowerControlHub is an Arduino-based 12V power distribution and control system for marine and off-grid applications. It provides configurable fused relay switching, multi-protocol sensor telemetry, WiFi/BLE/MQTT connectivity, and a companion touchscreen control panel — all managed through a unified serial command protocol and EEPROM-persisted configuration.

> Current firmware version: **0.9.1.x** — see [`FirmwareVersion.h`](PowerControlHub/FirmwareVersion.h).

---

## Table of Contents

- [Overview](#overview)
- [Prototype PCBs](#prototype-pcbs)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Board Support](#board-support)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Subsystem Reference](#subsystem-reference)
- [Limitations and Known Issues](#limitations-and-known-issues)
- [Documentation](#documentation)
- [License](#license)




## Overview

PowerControlHub is a single-unit firmware target running on an Arduino Uno R4 WiFi or ESP32 NodeMCU-32. The Nextion touchscreen display is driven directly by the PCH controller over a dedicated hardware serial port (`Serial2` for GPS; display connected via `NextionControl`).

The firmware entry point is `PowerControlHubApp`, which owns all subsystem instances and wires them together at startup. Host and debug communication uses a text-based framed serial protocol:

```
<CMD>:<key>=<value>;<key>=<value>\n
```

Commands are grouped by prefix (`F`, `C`, `R`, `H`, `W`, `S`, `M`, `N`, `E`, `T`) and are available over serial, USB, and — where applicable — the HTTP REST API. See [Docs/Commands.md](Docs/Commands.md) for the full reference.

---

## Prototype PCBs

<a href="Docs/pcbway.md"><img src="Docs/img/pcbway.jpg" width="180" align="right" alt="Prototype PCBs for PowerControlHub were supported by PCBWay" /></a>
Prototype PCBs for **PowerControlHub** were supported by [PCBWay](https://www.pcbway.com), enabling rapid iteration from design to real-world testing.

See the [PCB Sponsorship & Manufacturing](Docs/pcbway.md) page for photos, build details, and ongoing testing.

---
## Key Features

### Relay Control
- Up to **8 independently configurable relay channels**, pin-assigned at runtime via configuration.
- **Linked relays** — any relay can be configured to mirror the state of another.
- **Default-on state** — individual relays can be initialised to active at boot.
- **Pulse mode** — relay activates for a configurable duration (seconds) then turns off automatically.
- **Named relays** — short name (5 chars) and long name (20 chars) stored in EEPROM for display use.
- **Action types** — relays can be designated as `Default`, `Horn` (COLREGS sound signal output), or `NightRelay` (automatic day/night switching driven by the LDR sensor).
- **Relay status bitmask** broadcast via `MessageBus` on every state change.

### Sensor Pipeline

All sensors inherit from `BaseSensor` and are registered with `SensorController`. The `RemoteSensor` class provides an extensibility point for sensors on external devices that push readings via serial.

| Sensor | Class | Poll Interval | Notes |
|---|---|---|---|
| **DHT11** | `Dht11SensorHandler` | 2 500 ms | Temperature (°C) and relative humidity (%). Raises `TemperatureSensorFailure` warning on read error; clears automatically on recovery. |
| **Light (LDR)** | `LightSensorHandler` | 30 000 ms | Rolling average over 10 readings. 3-reading debounce before day/night state change. Configurable ADC threshold (default 512). Drives a `NightRelay` automatically. |
| **Water Level** | `WaterSensorHandler` | Configurable | 15-reading rolling average. Active-pin corrosion-reduction control (power applied only during measurement). |
| **GPS** | `GpsSensorHandler` | Continuous | GY-GPS6MV2 (NMEA). Reports latitude, longitude, altitude, speed (km/h), course, cumulative distance. Provides UTC time for automatic RTC synchronisation. Raises `GpsFailure` warning if no valid fix for 30 seconds. |
| **System Diagnostics** | `SystemSensorHandler` | Configurable | Free heap memory, CPU usage percentage, WiFi/BT enabled state, SD log file size (bytes), system uptime (`d HH:MM:SS`), active warning count. |

Sensors expose data over Serial, WiFi REST, BLE, and MQTT. Custom local or remote sensors can be added with minimal boilerplate; see [Docs/Sensors.md](Docs/Sensors.md).

### Connectivity

#### WiFi
- Operates in **Access Point** or **Client** mode, selectable via `C12`.
- Implements **persistent HTTP connections** to reduce TCP handshake overhead on resource-constrained hardware.
- Supports up to **8 concurrent clients** (`MaxConcurrentClients`).
- Automatic reconnection with exponential backoff in Client mode.
- RSSI monitoring — raises `WeakWifiSignal` warning when signal falls below −70 dBm.
- Full REST API across 9 route groups:

| Route | Handler | Purpose |
|---|---|---|
| `/api/relay` | `RelayNetworkHandler` | Relay state read/write |
| `/api/sensor` | `SensorNetworkHandler` | Sensor telemetry |
| `/api/sound` | `SoundNetworkHandler` | COLREGS horn signals |
| `/api/system` | `SystemNetworkHandler` | System status, heartbeat, OTA |
| `/api/config` | `ConfigNetworkHandler` | Configuration read/write |
| `/api/warning` | `WarningNetworkHandler` | Warning bitmask access |
| `/api/timer` | `SchedulerNetworkHandler` | Scheduled event read |
| `/api/externalsensor` | `ExternalSensorNetworkHandler` | Remote sensor config |
| `/` | `WebIndexNetworkHandler` | Index page |

Uses a platform-abstraction interface (`IWifiRadio`) to isolate board-specific WiFi drivers from application logic.

#### Bluetooth BLE
- Available on boards with `BLUETOOTH_SUPPORT` defined.
- Exposes three GATT services via `BluetoothManager`:
  - `BluetoothSystemService` — system diagnostics characteristics.
  - `BluetoothSensorService` — sensor telemetry characteristics with notifications.
  - `BluetoothRelayService` — relay state read and write characteristics.
- Single-client model. Advertising restarts automatically after disconnect.
- No pairing or bonding. Device advertises as **"Power Control Hub"**.
- Uses `IBluetoothRadio` interface — higher-level code operates exclusively through the interface pointer, with runtime null-checks replacing compile-time guards.
- **Arduino Uno R4 WiFi constraint:** WiFi and BLE share the RF path on the ANNA-B112/ESP32-S3 modules; `BLUETOOTH_SUPPORT` and `WIFI_SUPPORT` are mutually exclusive on this board. Enforced by a compile-time `#error`. On ESP32, both can operate simultaneously.

#### MQTT
- Custom **MQTT 3.1.1** client implementation (`MQTTClient`) — no third-party MQTT library dependency.
- **Home Assistant auto-discovery** — all sensors and relays register as HA entities on connect.
- Configurable broker address (64 chars), credentials (32 chars each), device ID, discovery prefix, port (default 1883), and keepalive interval (default 60 s).
- Requires `WIFI_SUPPORT`. Guarded by `#if defined(WIFI_SUPPORT)` in `BoardConfig.h`.

### Warning System

A **32-bit bitmask** tracks up to 32 independent typed conditions within `WarningManager`.

- **Sensor failure aggregation**: any sensor warning (bits 21–31) automatically raises `SensorFailure` (bit 20). `SensorFailure` clears only when all sensor warnings are cleared.
- Warning state changes are broadcast on the `MessageBus` via `WarningChanged`.
- Full warning type reference: [Docs/Warnings.md](Docs/Warnings.md).

### Scheduler

Event-driven relay automation engine with EEPROM-persisted configuration.

- Up to **20 events** (6 on boards with ≤ 1 KB EEPROM) stored in `SchedulerSettings`.
- Each event specifies an independent **trigger**, **condition guard**, and **action**.

| Dimension | Supported Values |
|---|---|
| **Triggers** | `TimeOfDay`, `Sunrise`, `Sunset`, `Interval`, `DayOfWeek`, `Date` |
| **Conditions** | `TimeWindow`, `DayOfWeek`, `IsDark`, `IsDaylight`, `RelayIsOn`, `RelayIsOff` |
| **Actions** | `RelayOn`, `RelayOff`, `RelayToggle`, `RelayPulse`, `AllRelaysOn`, `AllRelaysOff`, `SetPinHigh`, `SetPinLow` |

Sunrise and sunset triggers are computed in real time from GPS coordinates by `SunCalculator`. The `IsDark` / `IsDaylight` conditions fall back to the LDR sensor when GPS is unavailable.

### Message Bus

`MessageBus` provides a **lightweight publish/subscribe event system** for internal component decoupling. No dynamic allocation occurs at publish time — subscriber lists use function-local statics.

Key events: `WarningChanged`, `RelayStatusChanged`, `TemperatureUpdated`, `HumidityUpdated`, `LightSensorUpdated`, `WaterLevelUpdated`, `GpsLocationUpdated`, `GpsAltitudeUpdated`, `GpsSpeedUpdated`, `WifiConnectionStateChanged`, `WifiSignalStrengthChanged`, `SystemMetricsUpdated`. Full reference: [Docs/MessageBus.md](Docs/MessageBus.md).

### Over-the-Air Updates (OTA)

- Available on **ESP32 + `WIFI_SUPPORT` + `OTA_AUTO_UPDATE`** only.
- Polls the GitHub Releases API (`k3ldar/PowerControlHub`) every **24 hours**.
- Downloads asset `PowerControlHub-esp32-v{major}.{minor}.{patch}.{build}.bin` with SHA-256 checksum verification.
- TLS connections use pinned root CA certificates (Sectigo E46 for `api.github.com`; ISRG Root X1 for `assets.githubusercontent.com`).
- **Auto-apply is disabled by default.** Enable via `F12:apply=1` or the `OtaFlagAutoApply` bit in `SystemHeader::reserved[0]`.
- OTA state (`idle`, `checking`, `available`, `downloading`, `rebooting`, `failed`, `uptodate`) is queryable at any time via `F13`.

### Diagnostics and Logging

- **SD card logging** via `SdCardLogger` / `MicroSdDriver`. Configurable SPI pins (`C4`). Raises `SpiPinConfigError`, `SdCardMissing`, `SdCardError`, or `SdCardLowSpace` warnings as appropriate.
- Optional **SD card config loader** (`CARD_CONFIG_LOADER`) — reads `config.txt` at boot to override EEPROM settings without reflashing.
- **DS1302 RTC** (`RtcDS1302Driver`) with configurable pins (`C18`). Automatically synchronised from GPS UTC time on each valid GPS fix.
- **CPU usage monitoring** via `SystemCpuMonitor`. Available over serial, WiFi, and MQTT.
- **Boot counter** and **crash counter** tracked in `SystemHeader` with magic number validation (`0x53464201`).
- **RGB LED warning indicator** (HW479) driven on configurable pins (`C8`).

### Nextion Display

- `NavigationController` manages an ordered list of active pages, rebuilt dynamically when the location type changes — no reboot required.
- Maritime-specific pages (buoys, cardinal markers, flags, VHF radio, COLREGS sound signals) are included in the page list only when `LocationType` is `Power`, `Sail`, `Fishing`, or `Yacht`.
- Pages included: Splash, Home, Relay, Relay Settings, Environment, System, Warning, Settings, About, Buoys, Cardinal Markers, Flags, Moon Phase, Sound (Fog / Maneuvering / Overtaking / Emergency / Other / Signals), VHF (Channels / Distress / Radio).

### Security

- **API key and HMAC key** fields (32 chars each) are reserved in the `Config` struct for future HTTP authentication.

---

## Architecture

### Repository Structure

```
PowerControlHub/    Firmware source (relays, sensors, WiFi, BLE, MQTT, OTA, SD, Nextion display)
Docs/            Reference documentation
TestData/        Serial command test scripts
```

### Subsystem Dependency Graph

```
PowerControlHubApp
├── MessageBus                  Pub/sub event bus (no dynamic allocation)
├── RelayController             Relay switching, linking, pulse, action types
├── SoundController             COLREGS signal sequencer -> horn relay
├── SensorController            Sensor pipeline (local + remote)
│   ├── Dht11SensorHandler
│   ├── LightSensorHandler
│   ├── WaterSensorHandler
│   ├── GpsSensorHandler
│   ├── SystemSensorHandler
│   └── RemoteSensor (xN)
├── ConfigController            EEPROM config lifecycle
├── WarningManager              32-bit bitmask, sensor failure aggregation
├── ScheduleController          Event engine: trigger -> condition -> action
├── WifiController              WiFi lifecycle, HTTP REST API
│   └── WifiServer              Connection state machine, request routing
├── BluetoothController         BLE lifecycle (BLUETOOTH_SUPPORT)
│   └── BluetoothManager
│       ├── BluetoothSystemService
│       ├── BluetoothSensorService
│       └── BluetoothRelayService
├── MQTTController              MQTT 3.1.1 client, HA auto-discovery (MQTT_SUPPORT)
├── SdCardLogger                SD card log writer (SD_CARD_SUPPORT)
├── RtcDS1302Driver             DS1302 real-time clock
├── OtaManager                  GitHub OTA (OTA_AUTO_UPDATE + ESP32 + WIFI_SUPPORT)
├── BroadcastManager            Centralised serial broadcast to computer/debug port
├── NavigationController        Nextion page navigation (NEXTION_DISPLAY_DEVICE)
└── SerialCommandManager        Framed serial command dispatch (USB/debug)
```

### Interface Abstraction Pattern

WiFi and Bluetooth subsystems are each accessed through a platform-agnostic interface (`IWifiRadio`, `IBluetoothRadio`). Board-specific drivers (`R4WifiRadio`, `Esp32WifiRadio`, `BluetoothController`) are constructed only inside `PowerControlHubApp` under the appropriate `#if defined(...)` guard. All higher-level code operates through interface pointers with runtime null-checks, minimising preprocessor usage and enabling straightforward test doubles.

### Configuration Persistence

The `Config` struct is a versioned, packed POD serialised to EEPROM. The current layout version is **6** (`ConfigVersion6`). The `SystemHeader` (32 bytes) precedes the config payload and contains a magic number (`0x53464201`), boot count, crash counter, reset reason, OTA flags, and a 16-bit checksum. A config version mismatch triggers migration logic in `ConfigManager` and raises `DefaultConfigurationFuseBox`.

---

## Board Support

| Board | WiFi | BLE | MQTT | OTA | Notes |
|---|---|---|---|---|---|
| **Arduino Uno R4 WiFi** | Yes | Yes | Yes | No | WiFi and BLE are mutually exclusive (shared RF path) |
| **Arduino Uno R4 Minima** | No | No | No | No | Serial and sensor support only |
| **ESP32 NodeMCU-32** | Yes | Yes | Yes | Yes | Full feature support; WiFi and BLE simultaneous |

Board selection is controlled by a single `#define` in `Local.h` using the trailing-underscore convention. `BoardConfig.h` derives all capability flags, EEPROM limits, and enforces incompatible feature combinations at compile time.

---

## Installation

### Prerequisites

| Requirement | Details |
|---|---|
| **Arduino IDE** | 1.8.19 or Arduino IDE 2.x |
| **Git for Windows** | Standard installation |
| **Windows 10 / 11** | No special configuration required |

### Required Libraries (Arduino Library Manager)

| Library | Source |
|---|---|
| `SerialCommandManager` | Si Carter |
| `DHT sensor library` | Adafruit |
| `QMC5883LCompass` | Arduino Library Manager |
| `NextionControl` | Custom — copy `NextionControl/` folder to Arduino `libraries/` directory |

### Steps

1. Clone the repository:

   ```
   git clone https://github.com/k3ldar/PowerControlHub
   ```

2. Install all required libraries via the Arduino IDE Library Manager, and copy `NextionControl` manually.

3. Open `k3ldar/PowerControlHub.ino` in the Arduino IDE.

4. Edit `Local.h` to activate the correct board and feature flags (see [Configuration](#configuration)).

5. Select the target board and COM port, then upload.

Full setup walkthrough: [Docs/SETUP.md](Docs/SETUP.md).

---

## Configuration

All hardware-specific configuration is isolated in a single file: `PowerControlHub/Local.h`. No other file requires modification for a standard hardware setup.

### Board Selection

Exactly one board define must be active. Remove the trailing underscore to activate:

```cpp
#define ARDUINO_UNO_R4       // Arduino Uno R4 WiFi
#define ARDUINO_R4_MINIMA_   // Arduino Uno R4 Minima (inactive)
#define ESP32_NODE_32_       // ESP32 NodeMCU-32 (inactive)
```

### Feature Flags

| Define | Default | Approx. Flash | Description |
|---|---|---|---|
| `FUSE_BOX_CONTROLLER` | Active | — | Enables the full SFB subsystem stack |
| `WIFI_SUPPORT` | Active | — | WiFi controller and HTTP REST API |
| `MQTT_SUPPORT` | Active | ~40 KB | MQTT client and HA auto-discovery (requires `WIFI_SUPPORT`) |
| `BLUETOOTH_SUPPORT` | Inactive | ~144 KB | BLE services (mutually exclusive with WiFi on Uno R4) |
| `SD_CARD_SUPPORT` | Active | ~32 KB | SD card logger and SPI driver |
| `NEXTION_DISPLAY_DEVICE` | Active | ~52 KB | Nextion display page system |
| `OTA_AUTO_UPDATE` | Active | ~165 KB | GitHub OTA (ESP32 + WiFi only) |
| `LED_MANAGER` | Inactive | — | Arduino R4 WiFi LED matrix (experimental) |
| `CARD_CONFIG_LOADER` | Inactive | — | Load config overrides from SD `config.txt` at boot |

### Relay Count

```cpp
constexpr uint8_t ConfigRelayCount = 8;  // number of relay channels
```

Pins are assigned at runtime via the `R11` serial command and persisted to EEPROM. `0xFF` marks a pin as unassigned.

Full configuration reference: [Docs/Local.md](Docs/Local.md).

---

## Usage

### Serial / USB Commands (Quick Reference)

Commands follow the frame format: `<CMD>:<key>=<value>;<key>=<value>\n`

```
# Save config to EEPROM
C0

# Enable WiFi in client mode, set SSID and password
C11:v=1
C12:v=1
C13:YourSSID
C14:YourPassword
C0

# Switch relay 0 on / off
R3:i=0;v=1
R3:i=0;v=0

# Query all relay states
R2

# Read temperature and humidity
S0
S1

# Enable MQTT and set broker
M0:v=1
M1:192.168.1.100
C0

# Enable Home Assistant auto-discovery
M6:v=1

# Query OTA status (ESP32 only)
F13

# Check for firmware update
F12

# Apply firmware update immediately
F12:apply=1
```

Full command reference (all groups, all parameters): [Docs/Commands.md](Docs/Commands.md).

### HTTP REST API (WiFi)

```
# Relay control
POST /api/relay/R3?i=0&v=1

# Sensor data
GET /api/sensor/S0

# System status
GET /api/system/F2

# OTA status
GET /api/system/F13
```

---

## Subsystem Reference

| Document | Contents |
|---|---|
| [Docs/Commands.md](Docs/Commands.md) | Full serial and REST command reference (all groups F, C, R, H, W, S, M, N, E, T) |
| [Docs/Local.md](Docs/Local.md) | `Local.h` configuration reference: board selection, feature flags, pin layout |
| [Docs/Sensors.md](Docs/Sensors.md) | Sensor classes, poll intervals, MQTT channels, warning behaviour, extensibility guide |
| [Docs/Warnings.md](Docs/Warnings.md) | 32-bit warning type reference, `WarningManager` API, propagation model |
| [Docs/Wifi.md](Docs/Wifi.md) | WiFi architecture: connection state machine, persistent connection model, handler pattern |
| [Docs/BluetoothBle.md](Docs/BluetoothBle.md) | BLE service/characteristic reference, connection model, adding new services |
| [Docs/ScheduleEvents.md](Docs/ScheduleEvents.md) | Scheduler data structures, trigger/condition/action types, EEPROM layout |
| [Docs/MessageBus.md](Docs/MessageBus.md) | Event reference, publish/subscribe API |
| [Docs/SETUP.md](Docs/SETUP.md) | Prerequisites, library installation, build instructions |
| [Docs/BOM.md](Docs/BOM.md) | Bill of materials: hardware, fasteners, connectors, 3D-printed parts |

---

## Limitations and Known Issues

- **Arduino Uno R4 WiFi**: WiFi and BLE cannot operate simultaneously due to the shared RF hardware path. This is enforced at compile time.
- **Arduino Uno R4 Minima**: No radio support. Serial, sensors, and Nextion display only.
- **OTA updates**: Restricted to ESP32 targets with WiFi in client mode and `OTA_AUTO_UPDATE` defined. Not available on Arduino Uno R4.
- **Scheduler capacity**: Limited to 6 events on boards with 1 KB or less of EEPROM.
- **LED matrix manager** (`LED_MANAGER`): Experimental on Arduino R4 WiFi; not recommended for production use.
- **SD card support**: Marked experimental in `Local.h`. SPI pin configuration must be correct or `SpiPinConfigError` is raised.
- **Single BLE client**: Only one BLE central can be connected at a time. No pairing or bonding is implemented.

---

## Documentation

| Getting Started | Configuration | Connectivity |
|:---|:---|:---|
| [Setup Guide](Docs/SETUP.md) | [Local.h Reference](Docs/Local.md) | [WiFi Architecture](Docs/Wifi.md) |
| [Bill of Materials](Docs/BOM.md) | [Scheduled Events](Docs/ScheduleEvents.md) | [Bluetooth BLE](Docs/BluetoothBle.md) |
| [Sensor Reference](Docs/Sensors.md) | [Warning System](Docs/Warnings.md) | [Message Bus](Docs/MessageBus.md) |
| [Command Reference](Docs/Commands.md) | | |

---

## License

PowerControlHub is released under the **GNU General Public License v3.0**.  
See [LICENSE](LICENSE) for the full licence text.

Copyright &copy; 2025 Simon Carter (`s1cart3r@gmail.com`)