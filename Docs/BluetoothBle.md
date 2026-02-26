# SmartFuseBox – Bluetooth BLE

This document covers the Bluetooth BLE subsystem: component responsibilities, service and characteristic reference, connection lifecycle, and how to add a new service.

---

## Availability

BLE is available on boards that define `BLUETOOTH_SUPPORT` in `Local.h`.

> **⚠️ Arduino Uno R4 WiFi constraint:** The ANNA-B112 (BLE) and ESP32-S3 (WiFi) modules share the same RF path. `BLUETOOTH_SUPPORT` and `WIFI_SUPPORT` **cannot both be active** on this board — a compile-time `#error` enforces this. On ESP32, both can run simultaneously.

---

## Architecture

```
BluetoothController
└── BluetoothManager
    ├── BluetoothSystemService    (system diagnostics)
    ├── BluetoothSensorService    (sensor data)
    └── BluetoothRelayService     (relay state + control)
```

### `BluetoothController`

Top-level lifecycle owner. Constructed inside `SmartFuseBoxApp` and driven by configuration.

Key methods:

| Method | Description |
|---|---|
| `setEnabled(bool)` | Enables or disables BLE; tears down cleanly on disable |
| `applyConfig(const Config*)` | Reads `config.bluetoothEnabled` and calls `setEnabled()` |
| `isEnabled()` | Returns current enabled state |
| `loop()` | Must be called from the main loop; delegates to `BluetoothManager::loop()` |

On enable, `BluetoothController`:
1. Initialises the ArduinoBLE stack (`BLE.begin()`).
2. Constructs all three services.
3. Creates `BluetoothManager` and calls `begin("Smart Fuse Box")`.
4. Calls `notifyInitialized()` on `BluetoothSystemService`.
5. Raises `WarningType::BluetoothInitFailed` if any step fails.

### `BluetoothManager`

Coordinator — initialises the BLE stack, registers services, manages advertising, and drives the per-loop update cycle. Does not contain any domain logic.

Key behaviours:

- Calls `begin()` on each registered `BluetoothServiceBase` during setup.
- Calls `BLE.poll()` every loop iteration.
- Detects connect/disconnect transitions and automatically restarts advertising after a disconnect.
- Calls `loop(currentMillis)` on every service each iteration.

### `BluetoothServiceBase`

Abstract base class. All services inherit from it and implement:

| Method | Purpose |
|---|---|
| `begin()` | Create characteristics, initialise values, register with `BLE` |
| `getServiceUUID()` | Return the service UUID string |
| `getServiceName()` | Return a human-readable name for logging |
| `loop(unsigned long)` | Periodic work — send notifications, update cached values |
| `getCharacteristicCount()` | Return number of characteristics (for diagnostics) |
| `getBLEService()` | Return `void*` to the underlying `BLEService` (used by manager for advertising) |

---

## Connection Model

- **Single client** — only one BLE central can be connected at a time.
- **Advertising** — starts automatically on `begin()` and restarts automatically after every disconnect.
- **No pairing / bonding** — all characteristics are open; security is physical proximity.
- **Device name** — advertised as **"Smart Fuse Box"**.

---

## Service & Characteristic Reference

### System Service (`BluetoothSystemService`)

**Service UUID:** `4fafc201-1fb5-459e-0fcc-c5c9c331914b`

| Characteristic | UUID | Type | Properties | Update Interval | Description |
|---|---|---|---|---|---|
| Heartbeat | `beb5483e-36e1-4688-b7f5-ea07361b26a0` | `uint32` | Read, Notify | 1000 ms | Monotonically incrementing counter; confirms BLE stack is alive |
| Initialized | `beb5483e-36e1-4688-b7f5-ea07361b26a1` | `uint8` | Read, Notify | On event | `0xFF` at startup; set to `1` once the SFB firmware has finished initialising |
| Free Memory | `beb5483e-36e1-4688-b7f5-ea07361b26a2` | `uint32` | Read, Notify | 5000 ms | Available heap in bytes |
| CPU Usage | `beb5483e-36e1-4688-b7f5-ea07361b26a3` | `float` | Read, Notify | 2000 ms | CPU load as a percentage (0.0 – 100.0) |

---

### Sensor Service (`BluetoothSensorService`)

**Service UUID:** `4fafc201-1fb5-459e-1fcc-c5c9c331914b`

All characteristics update together every **1000 ms**. Values are read from `SensorCommandHandler`.

| Characteristic | UUID | Type | Properties | Description |
|---|---|---|---|---|
| Temperature | `beb5483e-36e1-4688-a7f5-ea07361b26a0` | `float` | Read, Notify | Ambient temperature in °C (DHT11) |
| Humidity | `beb5483e-36e1-4688-a7f5-ea07361b26a1` | `float` | Read, Notify | Relative humidity % (DHT11) |
| Bearing | `beb5483e-36e1-4688-a7f5-ea07361b26a2` | `float` | Read, Notify | Compass bearing in degrees |
| Compass Temp | `beb5483e-36e1-4688-a7f5-ea07361b26a3` | `float` | Read, Notify | Compass module temperature in °C |
| Speed | `beb5483e-36e1-4688-a7f5-ea07361b26a4` | `int` | Read, Notify | Speed in km/h |
| Water Level | `beb5483e-36e1-4688-a7f5-ea07361b26a5` | `int` | Read, Notify | Water level ADC rolling average |
| Water Pump Active | `beb5483e-36e1-4688-a7f5-ea07361b26a6` | `byte` | Read, Notify | `1` = pump active, `0` = inactive |

---

### Relay Service (`BluetoothRelayService`)

**Service UUID:** `4fafc201-1fb5-459e-2fcc-c5c9c331914b`

| Characteristic | UUID | Type | Properties | Description |
|---|---|---|---|---|
| Relay States | `9b9f0002-3c4a-4d9a-a6c1-11aa22bb33cc` | `byte[]` | Read, Notify | One byte per relay (length = relay count). `0` = off, `1` = on. Notified on any relay state change. |
| Relay Set | `9b9f0003-3c4a-4d9a-a6c1-11aa22bb33cc` | `byte[2]` | WriteWithoutResponse | Write `[relayIndex, state]` to change a relay. `relayIndex` is 0-based; `state` is `0` (off) or `1` (on). |

**Write example** — turn relay 2 on:

```
Write to Relay Set: 0x02 0x01
```

Relay States notifies automatically after the change is applied so clients see the updated state without polling.

---

## Enabling BLE in `Local.h`

```cpp
// Remove the trailing underscore to activate
#define BLUETOOTH_SUPPORT

// Ensure WiFi is NOT also defined on Arduino Uno R4
// #define WIFI_SUPPORT_   ← disabled
```

---

## Configuration

BLE is enabled or disabled at runtime via the `C10` serial command or the BCP touchscreen, which sets `config.bluetoothEnabled`. `BluetoothController::applyConfig()` is called whenever the config changes and tears down or brings up the BLE stack accordingly.

```
C10:v=1   ← enable BLE
C10:v=0   ← disable BLE (requires restart to fully release hardware)
```

---

## Warnings

| Warning | Trigger | Cleared by |
|---|---|---|
| `BluetoothInitFailed` | `BLE.begin()` returns false, service `begin()` fails, or device name is empty | Manually |

---

## Adding a New BLE Service

**Step 1 – Create the service class**

Inherit from `BluetoothServiceBase` and place the file alongside the other services:

```cpp
#pragma once
#include <ArduinoBLE.h>
#include "BluetoothServiceBase.h"

class BluetoothMyService : public BluetoothServiceBase
{
public:
    static constexpr const char* ServiceUuid = "4fafc201-1fb5-459e-3fcc-c5c9c331914b";
    static constexpr const char* MyValueUuid = "beb5483e-36e1-4688-c7f5-ea07361b26a0";

    bool begin() override
    {
        _service = new BLEService(ServiceUuid);
        _charValue = new BLEIntCharacteristic(MyValueUuid, BLERead | BLENotify);
        _service->addCharacteristic(*_charValue);
        BLE.addService(*_service);
        return true;
    }

    void loop(unsigned long currentMillis) override
    {
        if (currentMillis - _lastUpdate >= 2000)
        {
            _charValue->writeValue(42);
            _lastUpdate = currentMillis;
        }
    }

    const char* getServiceUUID() const override { return ServiceUuid; }
    const char* getServiceName() const override { return "MyService"; }
    uint8_t getCharacteristicCount() const override { return 1; }
    void* getBLEService() override { return _service; }

private:
    BLEService* _service = nullptr;
    BLEIntCharacteristic* _charValue = nullptr;
    unsigned long _lastUpdate = 0;
};
```

**Step 2 – Register the service in `BluetoothController.h`**

Add a member, instantiate it in `enableInternal()`, and include it in the `_services[]` array:

```cpp
// Add member
BluetoothMyService* _myService;

// In enableInternal():
_myService = new BluetoothMyService();

_serviceCount = 4U;   // was 3
_services = new BluetoothServiceBase*[_serviceCount];
_services[0] = _systemService;
_services[1] = _sensorService;
_services[2] = _relayService;
_services[3] = _myService;   // <-- add here
```

`BluetoothManager` will automatically call `begin()` and `loop()` on it without any further changes.