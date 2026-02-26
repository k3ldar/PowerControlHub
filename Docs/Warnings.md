# SmartFuseBox – Warning System

This document covers the warning system used across both the SmartFuseBox (SFB) and Boat Control Panel (BCP) controllers, including all warning types, the `WarningManager` API, propagation between devices, and how to add a new warning.

---

## Overview

The warning system tracks abnormal conditions across the entire system using a **32-bit bitmask**. Each bit represents a single named `WarningType`. Up to 32 independent warnings can be active simultaneously.

`WarningManager` separates warnings into two buckets:

| Bucket | Description |
|---|---|
| **Local** | Raised by the current device (`_localWarnings`) |
| **Remote** | Received from the connected peer via heartbeat or `W2` command (`_remoteWarnings`) |

`isWarningActive()` and `getActiveWarningsMask()` always return the **combined** view (local OR remote).  
`getLocalWarningsMask()` and `getRemoteWarningsMask()` return each bucket independently.

---

## Warning Type Reference

Bit ranges are reserved as follows:
- **Bits 0–19** — System warnings
- **Bits 20–31** — Sensor warnings (raising any of these also auto-raises `SensorFailure`)

| Bit | Hex Value | Constant | Display String | Raised by | Cleared by |
|---|---|---|---|---|---|
| 0 | `0x00000001` | `DefaultConfigurationFuseBox` | Fuse box Default Configuration | EEPROM config load fails at boot | Manually / successful config load |
| 1 | `0x00000002` | `DefaultConfigurationControlPanel` | Control Panel Default Configuration | BCP EEPROM config load fails at boot | Manually |
| 2 | `0x00000004` | `ConnectionLost` | Connection Lost To Fuse Box | No heartbeat ACK received within timeout (3000 ms) | Automatically on next `ACK:F0=ok` |
| 3 | `0x00000008` | `HighCompassTemperature` | High Compass Temperature | Compass module temperature exceeds threshold | Manually / when temperature falls |
| 4 | `0x00000010` | `LowBattery` | Low Battery | Battery voltage monitor | Manually |
| 5 | `0x00000020` | `BluetoothInitFailed` | Bluetooth Init Failed | BLE stack fails to initialise | Manually |
| 6 | `0x00000040` | `WifiInitFailed` | Wifi Init Failed | `WifiServer` creation fails | Manually |
| 7 | `0x00000080` | `WifiInvalidConfig` | Wifi Invalid Config | SSID empty or port is zero | Manually / on valid config applied |
| 8 | `0x00000100` | `WeakWifiSignal` | Weak Wifi Signal | RSSI drops below −70 dBm | Automatically when signal recovers |
| 9 | `0x00000200` | `SyncFailed` | Synchronization Failed | Config sync with BCP fails or times out | Automatically on next successful sync |
| 10 | `0x00000400` | `SdCardError` | SD Card Error | SD card read/write error | Manually |
| 11 | `0x00000800` | `SdCardMissing` | SD Card Not Found | SD card not detected at initialisation | Automatically when card is inserted |
| 12 | `0x00001000` | `SdCardLowSpace` | SD Card Low Space | Free space falls below 10% | Automatically when space recovers |
| 13–19 | — | *(reserved)* | — | — | — |
| 20 | `0x00100000` | `SensorFailure` | Sensor Failure | **Auto-raised** when any sensor warning (bits 21–31) is raised | **Auto-cleared** when all sensor warnings are cleared |
| 21 | `0x00200000` | `TemperatureSensorFailure` | Temperature Sensor Failure | DHT11 returns a read error | Automatically on next successful DHT11 read |
| 22 | `0x00400000` | `CompassFailure` | Compass Failure | Compass module fails to initialise | Manually |
| 23 | `0x00800000` | `GpsFailure` | GPS Module Failure | No valid GPS fix received for 30 seconds | Automatically on next valid GPS fix |
| 24–31 | — | *(reserved)* | — | — | — |

---

## Automatic Behaviours

### SensorFailure Aggregation

Raising **any** sensor warning (bits 20–31) automatically raises `SensorFailure` (bit 20) if it is not already active. Clearing a sensor warning automatically clears `SensorFailure` **only** when no other sensor warnings remain. This means consumers can check `SensorFailure` as a single indicator that something is wrong with sensors, without polling every individual sensor flag.

### ConnectionLost

`WarningManager` manages the heartbeat internally. It sends an `F0` command at the configured interval (`HeartbeatIntervalMs` = 1000 ms) and raises `ConnectionLost` if no `ACK:F0=ok` is received within the timeout (`HeartbeatTimeoutMs` = 3000 ms). The warning clears automatically the moment a valid heartbeat ACK arrives.

On the BCP, when a connection is re-established after being lost, the current time is automatically re-synced to the SFB via `F6`.

---

## WarningManager API

```cpp
// Raise a warning (no-op if already active)
warningManager.raiseWarning(WarningType::WifiInvalidConfig);

// Clear a warning (no-op if not active)
warningManager.clearWarning(WarningType::WifiInvalidConfig);

// Clear all warnings (local and remote)
warningManager.clearAllWarnings();

// Check if any warnings are active (local or remote)
if (warningManager.hasWarnings()) { }

// Check a specific warning (checks local AND remote)
if (warningManager.isWarningActive(WarningType::ConnectionLost)) { }

// Get the full combined bitmask (local | remote)
uint32_t mask = warningManager.getActiveWarningsMask();

// Get masks separately
uint32_t local  = warningManager.getLocalWarningsMask();
uint32_t remote = warningManager.getRemoteWarningsMask();

// Called from the main loop — drives heartbeat and connection state
warningManager.update(millis());

// Called from AckCommandHandler when ACK:F0=ok arrives
warningManager.notifyHeartbeatAck();

// Called from WarningCommandHandler when W2 arrives from the peer
warningManager.updateRemoteWarnings(remoteMask);
```

---

## Warning Propagation Between Devices

Warnings flow between SFB and BCP through two mechanisms:

### 1. Heartbeat (`F0`)

Every heartbeat includes the sender's local warning bitmask as a hex parameter:

```
F0:w=0x00000200    ← SFB reports SyncFailed
F0:w=0x00;t=...   ← BCP reports no warnings + timestamp
```

The receiver's `WarningCommandHandler` (via `AckCommandHandler`) calls `updateRemoteWarnings()` with the parsed mask.

### 2. Live broadcast (`W2`)

Whenever a **local** warning is raised or cleared, `WarningManager` immediately sends a `W2` command to the peer so the change is reflected without waiting for the next heartbeat:

```
W2:0x00000004=1   ← ConnectionLost raised
W2:0x00000004=0   ← ConnectionLost cleared
```

The receiving device's `WarningCommandHandler` handles `W2` and calls `raiseWarning()` or `clearWarning()` accordingly.

---

## Serial Commands

All warning commands are handled by `WarningCommandHandler`. Commands are available on both the LINK and COMPUTER serial ports.

| Command | ID | Parameters | Description |
|---|---|---|---|
| Warnings Active | `W0` | none | Returns the combined active warning bitmask as hex (`v=0x...`) |
| Warnings List | `W1` | none | Returns the combined active warning bitmask as hex (`v=0x...`) |
| Warning Status | `W2` | `<hex_flag>=<ignored>` | Returns `1` (active) or `0` (inactive) for a specific warning flag |
| Warnings Clear | `W3` | none | Clears all local and remote warnings |
| Warning Add/Clear | `W4` | `<hex_flag>=<0\|1>` | Raises (`1`) or clears (`0`) a specific warning by its hex bit value |

**Examples:**

```
W0                      → ACK:W0=ok;v=0x00000200
W2:0x00000004=?         → ACK:W2=ok;0x00000004=0
W4:0x00000008=1         → raises HighCompassTemperature
W4:0x00000008=0         → clears HighCompassTemperature
W3                      → clears all warnings
```

Warning types are passed as **hex bit-flag values** (e.g. `0x4` for `ConnectionLost`). The value must be a single set bit (power of 2); multi-bit values are rejected.

---

## HTTP API

The `WarningNetworkHandler` exposes one endpoint:

```
GET /api/warning/W1
```

**Response:**

```json
{ "warning": { "active": "0x200" } }
```

The value is the combined (`local | remote`) warning bitmask in hex.

---

## MQTT

The `SystemSensorHandler` publishes the **active warning count** (number of set bits) as an MQTT sensor channel:

| Channel | Slug | Topic suffix | Value |
|---|---|---|---|
| Sys Warnings | `warning_count` | `sensor/warning_count/state` | Integer count of active warnings |

This updates every 2 500 ms alongside the other system diagnostics channels.

---

## BCP-Specific Behaviour

On the Boat Control Panel, `WarningManager` drives two additional outputs when compiled with `BOAT_CONTROL_PANEL`:

| Output | Behaviour |
|---|---|
| **RGB LED** (`RgbLedFade`) | Pulses the warning status LED whenever any warning is active |
| **Tone** (`ToneManager`) | Plays the configured "bad" tone immediately when a new warning appears; repeats at the configured interval while any warning remains active |

---

## Adding a New Warning

**Step 1 – Add the bit flag to `WarningType.h`**

Choose the next unused bit in the appropriate range (0–19 for system, 20–31 for sensor) and add the entry to the `WarningType` enum:

```cpp
enum class WarningType : uint32_t
{
    // ...existing entries...
    MyNewWarning = 1UL << 13,   // 0x00002000
};
```

**Step 2 – Add the display string in `WarningType.h`**

Add a `PROGMEM` string and update the `WT_13` slot in the strings array:

```cpp
static const char WT_13[] PROGMEM = "My New Warning Description";
```

**Step 3 – Raise and clear the warning where appropriate**

Inject `WarningManager*` into the subsystem that detects the condition and call `raiseWarning` / `clearWarning`:

```cpp
// Raise when condition is detected
if (somethingWentWrong && !_warningManager->isWarningActive(WarningType::MyNewWarning))
{
    _warningManager->raiseWarning(WarningType::MyNewWarning);
}

// Clear when condition recovers
if (recovered && _warningManager->isWarningActive(WarningType::MyNewWarning))
{
    _warningManager->clearWarning(WarningType::MyNewWarning);
}
```

> **Note:** If this is a sensor warning (bit 20+), `SensorFailure` will be raised and cleared automatically — no extra code needed.