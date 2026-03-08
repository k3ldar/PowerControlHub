# Scheduled Events

The scheduler allows automated relay control based on time, date, GPS-derived sun position, or relay state. Up to **20 events** (6 on boards with ≤ 1 KB EEPROM) can be stored in EEPROM and survive power cycles.

---

## Feature Flag

Scheduler support is enabled automatically when the board has at least 1 KB of EEPROM:

```cpp
#if EEPROM_CAPACITY_BYTES >= 1024
#define SCHEDULER_SUPPORT
#endif
```

All scheduler code — config storage, command handling, network API, and runtime processing — is compiled only when `SCHEDULER_SUPPORT` is defined.

---

## Architecture

```
Serial / WiFi
     │
     ▼
SchedulerCommandHandler          Parses T0–T6 serial/USB commands
     │  writes to
     ▼
Config (EEPROM)                  Persists ScheduledEvent array via ConfigManager
     │  read by
     ▼
ScheduleController               Runtime engine — called every loop()
  ├── isTriggerDue()             Evaluates trigger against current time / millis()
  ├── isConditionMet()           Evaluates guard condition
  ├── executeAction()            Drives RelayController
  └── updatePulses()             Turns off relays when pulse duration expires

MessageBus
  ├── GpsLocationUpdated  ──────► ScheduleController (lat/lon for sun calculation)
  └── LightSensorUpdated  ──────► ScheduleController (day/night fallback)

SchedulerNetworkHandler          Read-only WiFi API — GET /api/timer/{T0|T1}
```

### Key Classes

| Class | File | Responsibility |
|---|---|---|
| `ScheduleController` | `ScheduleController.h/.cpp` | Runtime event evaluation and relay execution |
| `SchedulerCommandHandler` | `SchedulerCommandHandler.h/.cpp` | Serial command handling (T0–T6) |
| `SchedulerNetworkHandler` | `SchedulerNetworkHandler.h/.cpp` | Read-only WiFi JSON API |

---

## Data Structures (`Config.h`)

### `ScheduledEvent`

Each event is a packed struct stored inside `SchedulerSettings`:

```cpp
struct ScheduledEvent {
    bool                enabled;
    TriggerType         triggerType;
    uint8_t             triggerPayload[4];    // type-specific bytes b0..b3
    ConditionType       conditionType;
    uint8_t             conditionPayload[4];  // type-specific bytes b0..b3
    SchedulerActionType actionType;
    uint8_t             actionPayload[4];     // type-specific bytes b0..b3
    uint8_t             reserved[8];
};
```

### `SchedulerSettings`

```cpp
struct SchedulerSettings {
    bool           isEnabled;    // global on/off switch
    uint8_t        eventCount;   // number of configured slots
    ScheduledEvent events[ConfigMaxScheduledEvents];
};
```

`SchedulerSettings` is embedded in the top-level `Config` struct and persisted to EEPROM via `C0`.

---

## Triggers

A trigger defines **when** an event fires. Payload bytes are little-endian where multi-byte values are used.

| Type | Value | Payload b0..b3 | Notes |
|---|---|---|---|
| `None` | `0` | — | Event never fires automatically |
| `TimeOfDay` | `1` | `b0`=hour (0–23), `b1`=minute (0–59) | Fires once per day at the specified time |
| `Sunrise` | `2` | `b0..b1`=int16 offset minutes | Fires at sunrise ± offset. Requires GPS fix. |
| `Sunset` | `3` | `b0..b1`=int16 offset minutes | Fires at sunset ± offset. Requires GPS fix. |
| `Interval` | `4` | `b0..b1`=uint16 interval minutes | Fires repeatedly every N minutes. Arms on first evaluation; does not fire immediately. |
| `DayOfWeek` | `5` | `b0`=bitmask | Fires at midnight (00:00) on matching days |
| `Date` | `6` | `b0`=day (1–31), `b1`=month (1–12) | Fires at midnight (00:00) on a specific calendar date |

**DayOfWeek bitmask:** `bit0`=Mon, `bit1`=Tue, `bit2`=Wed, `bit3`=Thu, `bit4`=Fri, `bit5`=Sat, `bit6`=Sun

| Days | Bitmask |
|---|---|
| Monday only | `1` |
| Mon–Fri (weekdays) | `31` |
| Sat–Sun (weekends) | `96` |
| Every day | `127` |

**Sunrise/Sunset offset:** a signed 16-bit integer stored little-endian in `b0..b1`. Positive = after, negative = before. Example: 10 minutes after sunrise → `b0=10, b1=0`. 30 minutes before sunset → store `-30` as `b0=0xE2, b1=0xFF`.

---

## Conditions

A condition is an optional guard evaluated at trigger time. If the condition is not met the action is skipped; the trigger is still latched for the current minute to prevent repeated re-evaluation. Use `None` (type `0`) for unconditional events.

| Type | Value | Payload b0..b3 | Notes |
|---|---|---|---|
| `None` | `0` | — | Action always fires when trigger matches |
| `TimeWindow` | `1` | `b0`=start_hour, `b1`=start_min, `b2`=end_hour, `b3`=end_min | Action only fires within the time window. Windows spanning midnight are supported. |
| `DayOfWeek` | `2` | `b0`=bitmask (same layout as trigger) | Action only fires on matching days |
| `IsDark` | `3` | — | True when current time is after sunset or before sunrise |
| `IsDaylight` | `4` | — | True when current time is between sunrise and sunset |
| `RelayIsOn` | `5` | `b0`=relay index (0-based) | True when the specified relay is currently on |
| `RelayIsOff` | `6` | `b0`=relay index (0-based) | True when the specified relay is currently off |

`IsDark` and `IsDaylight` use computed sun times when a GPS fix is available. When no GPS fix has been received they fall back to the `LightSensorUpdated` MessageBus event (light sensor reading).

---

## Actions

| Type | Value | Payload b0..b3 | Notes |
|---|---|---|---|
| `None` | `0` | — | Trigger fires but nothing happens |
| `RelayOn` | `1` | `b0`=relay index | Turns the relay on |
| `RelayOff` | `2` | `b0`=relay index | Turns the relay off |
| `RelayToggle` | `3` | `b0`=relay index | Inverts the current relay state |
| `RelayPulse` | `4` | `b0`=relay index, `b1..b2`=uint16 duration seconds | Turns relay on for the specified duration, then turns it off automatically |
| `AllRelaysOn` | `5` | — | Turns every relay on |
| `AllRelaysOff` | `6` | — | Turns every relay off |

All relay indices are 0-based (`0` = Relay 1 … `7` = Relay 8).

---

## Serial Commands (T0–T6)

Handled by `SchedulerCommandHandler`. All commands follow the standard `ACK:<cmd>=ok` / `ACK:<cmd>=<error>` response pattern.

| Command | Example | Description |
|---|---|---|
| `T0` | `T0` | List all events. Returns `count=N;s=0,1,0,…` (comma-separated enabled states for all 20 slots) |
| `T1` | `T1:v=2` | Get full detail of event at index `v` |
| `T2` | `T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0` | Set or update event at index `i`. See format below. Save with `C0`. |
| `T3` | `T3:v=2` | Delete event at index `v`. Save with `C0`. |
| `T4` | `T4:i=2;v=1` | Enable (`v=1`) or disable (`v=0`) event at index `i`. Save with `C0`. |
| `T5` | `T5` | Delete all events. Save with `C0`. |
| `T6` | `T6:v=2` | Immediately execute the action of event `v`, bypassing trigger and condition. Useful for testing. |

### T2 Parameter Format

```
T2:i=<index>;e=<enabled>;t=<type,b0,b1,b2,b3>;c=<type,b0,b1,b2,b3>;a=<type,b0,b1,b2,b3>
```

Each of `t`, `c`, `a` is a comma-separated 5-value string: the first value is the enum type, followed by four payload bytes.

---

## WiFi API

Handled by `SchedulerNetworkHandler`. Read-only (GET). Returns JSON.

| Route | Description |
|---|---|
| `GET /api/timer/T0` | List all events — count and per-slot enabled states |
| `GET /api/timer/T1?v=<index>` | Full detail of a single event |

### Example Responses

**T0:**
```json
{
  "success": true,
  "schedule": {
    "count": 2,
    "slots": [1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  }
}
```

**T1?v=0:**
```json
{
  "success": true,
  "event": {
    "i": 0,
    "enabled": 1,
    "trigger":   { "type": 1, "b": [18, 30, 0, 0] },
    "condition": { "type": 0, "b": [0,  0,  0, 0] },
    "action":    { "type": 2, "b": [2,  0,  0, 0] }
  }
}
```

The scheduler overview is also included in the main status response at `GET /`.

---

## Runtime Engine (`ScheduleController`)

`ScheduleController` is owned by `SmartFuseBoxApp` and wired during `setup()` / `loop()`.

### Lifecycle

```cpp
// setup()
_scheduleController.begin();   // subscribes to MessageBus events

// loop()
_scheduleController.update(millis());
```

### Evaluation Cycle

`update()` runs once per second (controlled by `ScheduleCheckIntervalMs = 1000 ms`). Each call:

1. **`updatePulses()`** — checked every tick (not throttled) to turn off relays precisely when a `RelayPulse` duration expires.
2. Guards: returns early if `DateTimeManager::isTimeSet()` is false, or if `cfg->scheduler.isEnabled` is false.
3. Recomputes sunrise/sunset via the NOAA simplified algorithm when the calendar date changes.
4. Iterates every enabled event slot and calls `isTriggerDue()` → `isConditionMet()` → `executeAction()`.

### Trigger Latching

For time-based triggers (`TimeOfDay`, `Sunrise`, `Sunset`, `DayOfWeek`, `Date`) the last-fired day and minute are recorded **when the trigger is seen**, even if the condition fails. This prevents repeated evaluations within the same minute and ensures each event fires at most once per occurrence.

`Interval` triggers manage their own re-arm timestamp inside `isTriggerDue()` independently.

### Sun Position Calculation

When a GPS fix is available (received via `GpsLocationUpdated` on the MessageBus), `ScheduleController` computes local sunrise and sunset times once per day using the NOAA simplified solar algorithm:

1. Julian Day Number (Gregorian calendar)
2. Mean solar noon
3. Solar mean anomaly
4. Equation of the centre
5. Ecliptic longitude
6. Solar transit
7. Declination
8. Hour angle at −0.833° (atmospheric refraction + solar disc)
9. Julian dates of rise/set
10. Convert to UTC minutes from midnight
11. Apply timezone offset from `DateTimeManager::getTimezoneOffset()`

Accuracy is typically within 1–2 minutes. For polar latitudes where the sun does not rise or set, `_sunriseMinutes` and `_sunsetMinutes` are set to `SunTimeUnknown (-1)` and `Sunrise`/`Sunset` triggers will not fire.

### Per-Event Runtime State

State is held in stack-allocated arrays indexed by event slot. It is **not persisted** — the system starts fresh on every boot.

| Array | Type | Purpose |
|---|---|---|
| `_lastFiredDay` | `uint32_t[20]` | Packed YYYYMMDD of last trigger evaluation |
| `_lastFiredMinute` | `uint16_t[20]` | Minute-of-day (0–1439) of last trigger evaluation |
| `_lastIntervalFireMs` | `unsigned long[20]` | `millis()` timestamp of last interval fire |
| `_pulseStartMs` | `unsigned long[20]` | `millis()` when `RelayPulse` began |
| `_pulseDurMs` | `unsigned long[20]` | `RelayPulse` duration in milliseconds |
| `_pulseRelayIdx` | `uint8_t[20]` | Relay to turn off when pulse expires |
| `_pulseActive` | `bool[20]` | Whether a pulse is currently running for the slot |

---

## Prerequisites

| Requirement | Impact if missing |
|---|---|
| `SCHEDULER_SUPPORT` defined | No scheduler code compiled |
| `cfg->scheduler.isEnabled == true` | Events evaluated but none fire |
| `DateTimeManager::isTimeSet()` | `update()` returns early — no events fire |
| GPS fix (`GpsLocationUpdated`) | `Sunrise`/`Sunset` triggers will not fire; `IsDark`/`IsDaylight` conditions fall back to the light sensor |
| Light sensor (`LightSensorUpdated`) | `IsDark`/`IsDaylight` conditions default to `true` (daytime) when neither GPS nor sensor is available |

---

## Usage Examples

All examples must be followed by `C0` to persist to EEPROM.

**Turn Relay 3 off at 18:30 every day:**
```
T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0
C0
```

**Turn Relay 2 on 10 minutes after sunrise:**
```
T2:i=1;e=1;t=2,10,0,0,0;c=0,0,0,0,0;a=1,1,0,0,0
C0
```

**Toggle Relay 1 at sunrise, weekdays only:**
```
T2:i=2;e=1;t=2,0,0,0,0;c=2,31,0,0,0;a=3,0,0,0,0
C0
```

**Pulse Relay 5 for 10 seconds every 30 minutes:**
```
T2:i=3;e=1;t=4,30,0,0,0;c=0,0,0,0,0;a=4,4,10,0,0
C0
```

**Turn all relays off at midnight on Christmas Day:**
```
T2:i=4;e=1;t=6,25,12,0,0;c=0,0,0,0,0;a=6,0,0,0,0
C0
```

**Test event 0 immediately (bypass trigger and condition):**
```
T6:v=0
```

---

## Error Responses

| Message | Cause |
|---|---|
| `Scheduler not supported` | `SCHEDULER_SUPPORT` not defined on this board |
| `Config not available` | `ConfigManager::getConfigPtr()` returned null |
| `Index out of range` | Event index ≥ `ConfigMaxScheduledEvents` |
| `Slot is empty` | `T1` requested detail for an unconfigured slot |
| `Missing params` | Required parameters not supplied |
| `Invalid trigger type` | Trigger type value exceeds known range |
| `Invalid condition type` | Condition type value exceeds known range |
| `Invalid action type` | Action type value exceeds known range |
| `Invalid enabled value (0 or 1)` | `T4` received a value other than `0` or `1` |
