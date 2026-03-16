# SmartFuseBox – `Local.h` Configuration Reference

`Local.h` is the **single file a developer edits** to configure SmartFuseBox for their specific hardware. It controls board selection, feature flags, pin assignments, and relay count. No other file should need to change for a standard hardware setup.

> **Do not commit secrets or hardware-specific pin changes back to the main repository.** Treat `Local.h` as a local override file.

---

## Naming Convention

Throughout `Local.h`, the **trailing underscore** convention marks a define as inactive:

```cpp
#define ARDUINO_UNO_R4        // ✅ ACTIVE   — remove underscore to enable
#define ARDUINO_R4_MINIMA_    // ❌ INACTIVE — underscore means disabled
```

This lets all options stay visible in the file without needing to delete or comment anything out.

---

## 1. Board Selection

Exactly **one** board must be active. The compile-time `#error` guards will catch multiple active selections or no selection.

| Define | Board | WiFi | BLE | Notes |
|---|---|---|---|---|
| `ARDUINO_UNO_R4` | Arduino Uno R4 WiFi | ✅ | ✅ | WiFi and BLE are mutually exclusive — see [Connectivity Constraints](#connectivity-constraints) |
| `ARDUINO_R4_MINIMA` | Arduino Uno R4 Minima | ❌ | ❌ | Serial and sensor support only; `WIFI_SUPPORT` and `BLUETOOTH_SUPPORT` must not be defined |
| `ESP32_NODE_32` | ESP32 NodeMCU-32 or compatible | ✅ | ✅ | Full feature support; activating this also defines `ESP32` if the ESP32 toolchain hasn't already done so |

**To switch boards**, remove the trailing underscore from the target and add one to all others:

```cpp
// Switch from Uno R4 to ESP32
#define ARDUINO_UNO_R4_     // now inactive
#define ARDUINO_R4_MINIMA_  // still inactive
#define ESP32_NODE_32       // now active
```

---

## 2. Feature Flags

### `FUSE_BOX_CONTROLLER`

```cpp
#define FUSE_BOX_CONTROLLER
```

Includes all SmartFuseBox controller functionality — relay management, sensor pipeline, config system, serial command handlers, and connectivity subsystems. Remove this only if using the underlying components (e.g. `WifiController`, `MQTTClient`) in a completely different project.

---

### `WIFI_SUPPORT`

```cpp
#define WIFI_SUPPORT
```

Enables the `WifiController`, `WifiServer`, HTTP REST API, and all network command handlers. Only define this on boards with a WiFi radio (`ARDUINO_UNO_R4` or `ESP32_NODE_32`).

---

### `MQTT_SUPPORT`

```cpp
#if defined(WIFI_SUPPORT)
#define MQTT_SUPPORT
#endif
```

Enables the MQTT client, `MQTTController`, and all MQTT handlers including Home Assistant auto-discovery. Guarded inside `#if defined(WIFI_SUPPORT)` — MQTT requires an active WiFi connection and will not compile without it.

To disable MQTT while keeping WiFi active, comment out or remove the `#define MQTT_SUPPORT` line.

---

### `BLUETOOTH_SUPPORT`

```cpp
#define BLUETOOTH_SUPPORT_   // inactive by default
```

Enables the BLE stack (`BluetoothController`, `BluetoothManager`) and all three BLE services. Remove the trailing underscore to activate. See [Connectivity Constraints](#connectivity-constraints) for the WiFi conflict on Arduino Uno R4.

---

### `CARD_CONFIG_LOADER`

```cpp
#define CARD_CONFIG_LOADER_   // inactive by default — experimental
```

When active, the system reads `config.txt` from the SD card at boot and on card-insert, allowing configuration changes without reflashing. Requires the SD card SPI pins to be wired correctly. This feature is **experimental** and disabled by default.

---

## 3. Connectivity Constraints

On **Arduino Uno R4 WiFi**, the ANNA-B112 (BLE) and ESP32-S3 (WiFi) modules share the same RF path and **cannot run simultaneously**. A compile-time guard enforces this:

```cpp
#if defined(ARDUINO_UNO_R4) && defined(WIFI_SUPPORT) && defined(BLUETOOTH_SUPPORT)
#error "WIFI_SUPPORT and BLUETOOTH_SUPPORT cannot both be enabled on Arduino Uno R4."
#endif
```

| Board | WiFi + MQTT | BLE only | Both |
|---|---|---|---|
| Arduino Uno R4 WiFi | ✅ | ✅ | ❌ compile error |
| Arduino R4 Minima | ❌ | ❌ | ❌ |
| ESP32 NodeMCU-32 | ✅ | ✅ | ✅ |

---

## 4. Serial Initialisation Timeout

```cpp
constexpr unsigned long serialInitTimeoutMs = 300;
```

If `waitForConnection` is `true` when initialising serial, the firmware waits up to this many milliseconds for the USB serial connection before continuing with setup. Useful when debugging startup log messages that would otherwise be missed because the serial monitor opens after the board has already booted.

---

## 5. Sensor Pins

> For full per-board pin tables, ESP32 safety rules, and porting caveats see **[Docs/Pins.md](Pins.md)**.

Default pin assignments for the example sensors. Change these to match your wiring.

| Constant | Default | Connected to |
|---|---|---|
| `WaterSensorPin` | `A0` | Water level sensor analog output |
| `LightSensorAnalogPin` | `A1` | LDR analog output — read every poll and averaged over a rolling queue of 10 samples |
| `LightSensorPin` | `D3` | LDR digital pin — initialised as `INPUT`; analog reading is now the primary detection source |
| `WaterSensorActivePin` | `D8` | Water sensor power pin — pulsed `HIGH` before each reading to reduce corrosion |
| `Dht11SensorPin` | `D9` | DHT11 data line |

When adding new sensors, define additional pin constants here and use them when constructing the sensor handler in `SmartFuseBox.ino`.

---

## 6. SD Card SPI Pins

Used by `MicroSdDriver` for SD card access (logging and optional config loading).

| Constant | Default | SPI Signal |
|---|---|---|
| `SdCardCsPin` | `D10` | Chip Select |
| `SdCardMosiPin` | `D11` | MOSI |
| `SdCardMisoPin` | `D12` | MISO |
| `SdCardSckPin` | `D13` | Clock |

---

## 7. Relay Configuration

### `ConfigRelayCount`

```cpp
constexpr uint8_t ConfigRelayCount = 8;
```

The number of physical relays. Must match the number of entries in the `Relays[]` array. The system will not function with fewer than 1 relay. Set this to less than the number of defined pin constants if not all relays are populated.

### Relay Pins

```cpp
constexpr uint8_t Relay1 = D7;   // digital header
constexpr uint8_t Relay2 = D6;
constexpr uint8_t Relay3 = D5;
constexpr uint8_t Relay4 = D4;
constexpr uint8_t Relay5 = 19;   // analog header used in digital mode (A5)
constexpr uint8_t Relay6 = 18;   // A4
constexpr uint8_t Relay7 = 17;   // A3
constexpr uint8_t Relay8 = 16;   // A2
```

On Arduino Uno R4 / R4 Minima, analog header pins `A2`–`A5` are exposed as integer indices `16`–`19` when used in digital mode.

### `Relays[]` Array

```cpp
constexpr uint8_t Relays[ConfigRelayCount] = {
    Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8
};
```

The command layer addresses relays by 0-based index into this array — index 0 is relay 1, index 1 is relay 2, and so on. **Always keep `Relay1` at index 0** and ensure the array contains exactly `ConfigRelayCount` entries.

---

## 8. Compile-Time Guards

`Local.h` enforces valid configurations with `#error` directives that fail the build immediately with a clear message:

| Guard | Condition | Error message |
|---|---|---|
| WiFi + BLE on R4 | `ARDUINO_UNO_R4 && WIFI_SUPPORT && BLUETOOTH_SUPPORT` | "WIFI_SUPPORT and BLUETOOTH_SUPPORT cannot both be enabled on Arduino Uno R4." |
| Multiple boards | e.g. `ARDUINO_UNO_R4 && ESP32` | "Multiple board types defined." |
| No board | None of the three boards active | "No board type defined." |

---

## 9. Common Configuration Recipes

### Arduino Uno R4 WiFi with MQTT (default)

```cpp
#define ARDUINO_UNO_R4
#define ARDUINO_R4_MINIMA_
#define ESP32_NODE_32_
#define FUSE_BOX_CONTROLLER
#define WIFI_SUPPORT
// MQTT_SUPPORT is auto-enabled inside the WIFI_SUPPORT guard
#define BLUETOOTH_SUPPORT_    // inactive — mutually exclusive with WiFi on R4
#define CARD_CONFIG_LOADER_   // inactive
```

### Arduino Uno R4 WiFi with BLE (no MQTT)

```cpp
#define ARDUINO_UNO_R4
#define ARDUINO_R4_MINIMA_
#define ESP32_NODE_32_
#define FUSE_BOX_CONTROLLER
// #define WIFI_SUPPORT     ← comment out entirely
#define BLUETOOTH_SUPPORT   // active — WiFi is off so no conflict
#define CARD_CONFIG_LOADER_
```

### Arduino Uno R4 Minima (sensors + serial only)

```cpp
#define ARDUINO_UNO_R4_        // inactive
#define ARDUINO_R4_MINIMA      // active
#define ESP32_NODE_32_         // inactive
#define FUSE_BOX_CONTROLLER
// WIFI_SUPPORT, MQTT_SUPPORT, BLUETOOTH_SUPPORT must all be absent or inactive
#define CARD_CONFIG_LOADER_
```

### ESP32 (WiFi + BLE + MQTT)

```cpp
#define ARDUINO_UNO_R4_
#define ARDUINO_R4_MINIMA_
#define ESP32_NODE_32         // active
#define FUSE_BOX_CONTROLLER
#define WIFI_SUPPORT
// MQTT_SUPPORT auto-enabled
#define BLUETOOTH_SUPPORT     // also active — ESP32 supports both simultaneously
#define CARD_CONFIG_LOADER_
```