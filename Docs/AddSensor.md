# Adding a New Sensor Type

This guide lists every file that must be touched when adding a new local sensor type to PowerControlHub. It uses the Reed contact sensor (`S24`, `SensorIdList::ReedSensor` / `SensorType.ReedSwitch`) as the reference implementation.

## Overview

A sensor type spans two projects:

| Project | Path | Language |
|---|---|---|
| Firmware | `PowerControlHub\` | C++ (Arduino / ESP32) |
| App | `PowerControlHubApp\` | C# / .NET MAUI |

Configuration plumbing (add / remove / rename / pin / options) is **type-agnostic** and driven by the `SensorDescriptors` table in `SensorConfig.h`. Most of the integration work is metadata: describe the sensor's pins and options, then implement its runtime behaviour.

## Quick-reference touch-point table

| # | File | Change |
|---|---|---|
| 1 | `PowerControlHub\SystemDefinitions.h` | Add `SensorIdList` value + telemetry command constant |
| 2 | `PowerControlHub\SensorConfig.h` | Add `SensorTypeDescriptor` entry |
| 3 | `PowerControlHub\<NewSensor>Handler.h` | New handler class |
| 4 | `PowerControlHub\SensorFactory.h` | `#include` + factory `case` |
| 5 | `PowerControlHub\SensorNetworkHandler.cpp` | WiFi telemetry dispatch |
| 6 | `PowerControlHub\SensorCommandHandler.cpp/.h` | Serial query/push (only if polled over COM) |
| 7 | `PowerControlHub\WarningType.h` | New failure warning (only if needed) |
| 8 | `PowerControlHubApp\Models\SensorType.cs` | App enum mirror |
| 9 | `PowerControlHubApp\Internal\Constants.cs` | `SensorEnumXxx` constant |
| 10 | `PowerControlHubApp\Models\LocalSensorConfigModel.cs` | `TypeName` switch case |
| 11 | `PowerControlHubApp\Models\ExternalSensorConfigModel.cs` | `SensorIdNames` array entry |
| 12 | `PowerControlHubApp\Views\Templates\SensorTemplateSelector.cs` | Template mapping |
| 13 | `PowerControlHubApp\ViewModels\LocalSensorDetailViewModel.cs` | Fallback picker entry |
| 14 | `PowerControlHubApp\ViewModels\ExternalSensorDetailViewModel.cs` | Fallback picker entry |
| 15 | `PowerControlHubApp\Resources\Localization\AppResources.resx` (+ locales + `.Designer.cs`) | Display strings |
| 16 | `PowerControlHubApp\Models\Json\SensorsModel.cs` (+ optional XAML template) | Dashboard card fields |
| 17 | `Docs\Commands.md` | Command documentation |

---

## 1. Firmware

### 1.1 `SystemDefinitions.h`

Two additions:

1. A unique enum value in `SensorIdList`, inserted **before** `Count` (`None = 0xFF` stays last):

```cpp
enum class SensorIdList : uint8_t
{
    // ... existing values ...
    ReedSensor = 0x7,

    Count,
    None = 0xFF
};
```

2. A telemetry command ID constant. Pick the next free number after the existing `S7`–`S21` range (Binary Presence is `S22`, Voltage is `S23`, Reed is `S24`):

```cpp
constexpr char SensorReed[] = "S24";
```

### 1.2 `SensorConfig.h`

Add a `SensorTypeDescriptor` entry indexed by the new enum value. This table drives the MAUI config UI (pin slots, options, ranges, defaults) and PinGuard validation, so it is the single source of truth for how the sensor is configured over both COM and WiFi.

```cpp
[static_cast<size_t>(SensorIdList::ReedSensor)] = {
    .name = "Reed",
    .pins = {
        { "Sensor Pin", "gpio", 0, 39, 255, PinUse::Sensor },
        { "Active Pin", "gpio", 0, 39, 255, PinUse::Output },
        { "Unused", "none", 0, 0, 0, PinUse::Sensor },
        { "Unused", "none", 0, 0, 0, PinUse::Sensor },
    },
    .options1 = {
        { "NO/NC", "int8", 0, 1, 0, PinUse::Sensor },
        { "Unused", "none", 0, 0, 0, PinUse::Sensor },
    },
    .options2 = { { "Unused", "none", 0, 0, 0, PinUse::Sensor }, { "Unused", "none", 0, 0, 0, PinUse::Sensor } },
},
```

Notes:

- `pins` has `ConfigMaxSensorPins` (4) slots; `options1`/`options2` have 2 slots each.
- `type` is `"gpio"`, `"int8"`, `"int16"`, or `"none"`. Only `"gpio"` slots go through PinGuard.
- `defaultValue` `255` (`PinDisabled`) means "not fitted" for a pin.
- The existing `static_assert(std::size(SensorDescriptors) == SensorIdList::Count, ...)` will fail until you add this entry.

### 1.3 Create the handler class

Create `PowerControlHub\<NewSensor>Handler.h`. Derive from `BaseSensor` (and `BroadcastLoggerSupport` if the sensor sends debug/telemetry):

| Member | Purpose |
|---|---|
| `initialize()` | Set `pinMode`, initial state, optional output pin |
| `update()` | Poll the hardware; return the poll interval in ms; publish state changes |
| `formatStatusJson()` | Serialize current values; this becomes the sensor's dashboard JSON |
| `getSensorIdType()` | Return the new `SensorIdList` value |
| `getSensorType()` | Return `SensorType::Local` |
| `getSensorCommandId()` | Return the `SensorXxx` command constant |
| `getMqttChannelCount()/getMqttChannel()/getMqttValue()` | MQTT channel metadata (under `#if defined(MQTT_SUPPORT)`) |

For binary sensors, `formatStatusJson()` should emit a `"state":"detected"|"clear"` string — the app's `SensorsModel` maps these to red/green icons.

Self-broadcasting sensors call `sendCommand(SensorXxx, params, count)` from `update()` to push telemetry over serial/WiFi. See `ReedSensorHandler.h`, `BinaryPresenceSensor.cpp`, and `VoltageSensorHandler.h` for the pattern.

### 1.4 `SensorFactory.h`

1. Add `#include "<NewSensor>Handler.h"` at the top.
2. Add a `case SensorIdList::<NewSensor>:` inside `createOne()`. Validate the required pin (`entry.pins[0] != PinDisabled`), raise the appropriate `WarningType` on failure, and map `entry.pins`/`entry.options1`/`entry.options2` into the constructor arguments.

### 1.5 `SensorNetworkHandler.cpp`

Add the new command constant to the `S7`–`S24` telemetry dispatch condition so `/api/sensor` can query the sensor's status over WiFi:

```cpp
SystemFunctions::commandMatches(command, SensorReed)
```

### 1.6 `SensorCommandHandler.cpp` / `.h` (serial COM)

This centralized handler stores last values for `S7`–`S21` and answers no-param serial polls. Sensors such as Binary Presence (`S22`), Voltage (`S23`), and Reed (`S24`) **self-broadcast** via `BroadcastLoggerSupport::sendCommand` instead of storing state here.

- If the new sensor self-broadcasts and does **not** need a serial poll query, no change is required.
- If you want a no-param serial poll (e.g. `S24` returns the current value), add a query branch in `handleCommand()` and the command to `supportedCommands()`.

### 1.7 `WarningType.h` (conditional)

Only if the new sensor needs its own distinct failure warning. Add a bit flag and a `PROGMEM` string, then raise it from `SensorFactory::createOne()`. Otherwise reuse an existing warning (e.g. `SensorFailure` or `BinarySensorFailure`).

`Config.h` needs **no change** for a typical sensor — `SensorEntry` already provides `pins[4]`, `options1[2]`, and `options2[2]`.

---

## 2. MAUI app

The app mirrors the firmware enum and provides config UI and dashboard rendering.

### 2.1 `Models\SensorType.cs`

Add an enum value matching the firmware `SensorIdList` numeric value:

```csharp
ReedSwitch = 0x7,
```

### 2.2 `Internal\Constants.cs`

Add the numeric constant used by the config models:

```csharp
public const int SensorEnumReed = 7;
```

### 2.3 `Models\LocalSensorConfigModel.cs`

Add a `SensorEnumXxx => SensorTypeXxx` case to the `TypeName` switch.

### 2.4 `Models\ExternalSensorConfigModel.cs`

Add an entry to the `SensorIdNames` array at the index matching the enum value:

```csharp
SensorTypeReed,        // 7  Reed
```

### 2.5 `Views\Templates\SensorTemplateSelector.cs`

Map the new `SensorType` to an existing template (e.g. `BinaryPresenceTemplate`) or `GenericTemplate`:

```csharp
SensorType.ReedSwitch => BinaryPresenceTemplate ?? GenericTemplate,
```

### 2.6 View models (config picker fallback)

Add the picker label to the fallback list in **both**:

- `ViewModels\LocalSensorDetailViewModel.cs`
- `ViewModels\ExternalSensorDetailViewModel.cs`

```csharp
SensorTypeOptions.Add(SensorTypeReedPicker);
```

(This fallback is only used when the device's `?meta=1` descriptor cache is unavailable; otherwise the list is built from `SensorDescriptors`.)

### 2.7 Localization strings

Add `SensorTypeReed` ("Reed") and `SensorTypeReedPicker` ("Reed (7)") to:

- `Resources\Localization\AppResources.resx`
- each localized variant (e.g. `AppResources.de-DE.resx`, etc.)
- `AppResources.Designer.cs` (regenerated by Visual Studio from the `.resx`)

### 2.8 Dashboard card (optional)

- `Models\Json\SensorsModel.cs`: add computed properties that read the new sensor's fields from `ExtraFields` (parsed from `formatStatusJson()`).
- If a dedicated card is wanted, create `Views\Templates\<NewSensor>Template.xaml` (+ code-behind), register it in `Views\DashboardPage.xaml`, and add a `DataTemplate` property to `SensorTemplateSelector`.

`Services\DashboardConnection.cs` already casts the firmware `idType` to `SensorType` via `(SensorType)idType`, so no change is needed there as long as the enum value matches.

---

## 3. Documentation

Update `Docs\Commands.md`:

1. Add the new value to the `SensorIdList` table.
2. Add a field-mapping row for the sensor's pins/options under sensor configuration.
3. Add a telemetry row under `Sensor Telemetry — S7–S24` describing the params it emits.

---

## 4. Verification checklist

1. Firmware builds (Visual Micro / VS `PowerControlHub.ino` project).
2. MAUI app builds (`dotnet build` on `PowerControlHubApp.csproj`).
3. Sensor appears in `?meta=1` descriptors and can be added/configured over COM (`S0`–`S6`) and WiFi (`/api/sensorconfig`).
4. Telemetry appears in `/api/index` (`formatStatusJson`) and, if applicable, the serial telemetry query.
5. MQTT discovery publishes the expected channels (when `MQTT_SUPPORT` is enabled).