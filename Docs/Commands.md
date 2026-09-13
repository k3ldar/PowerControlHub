# PowerControlHub Command Reference

## Overview

This document describes every command the PowerControlHub (PCH) firmware understands.
Commands travel over two transports:

| Transport | Format | Direction |
|---|---|---|
| **Serial / USB** | `<CMD>:<key>=<val>;<key>=<val>\n` | Bidirectional |
| **WiFi REST** | `POST /api/<group>/<CMD>?<key>=<val>&…` | Client → PCH |

### Command-group prefix summary

| Prefix | Group | Handler | WiFi route |
|---|---|---|---|
| `F` | System | `SystemCommandHandler` | `/api/system` |
| `C` | Configuration | `ConfigCommandHandler` / `ConfigNetworkHandler` | `/api/config` |
| `R` | Relay control & config | `RelayCommandHandler` / `RelayNetworkHandler` | `/api/relay` |
| `H` | Sound / fog-horn signals | `SoundCommandHandler` / `SoundNetworkHandler` | `/api/sound` |
| `W` | Warnings | `WarningCommandHandler` / `WarningNetworkHandler` | `/api/warning` |
| `S` | Sensor config & telemetry | `SensorConfigCommandHandler` / `SensorCommandHandler` / `SensorNetworkHandler` | `/api/sensor` |
| `M` | MQTT configuration | `MQTTConfigCommandHandler` | `/api/config` |
| `N` | Nextion display config | `NextionConfigCommandHandler` / `ConfigNetworkHandler` | `/api/config` |
| `E` | External (remote) sensor config | `ExternalSensorConfigCommandHandler` / `ExternalSensorNetworkHandler` | `/api/externalsensor` |
| `T` | Timer / scheduler | `SchedulerCommandHandler` / `SchedulerNetworkHandler` | `/api/timer` |
| `ACK` | Acknowledgement | `AckCommandHandler` | — |

### General conventions

- All commands and parameters are **case-sensitive**.
- Serial frame format: `CMD:key=val;key=val\n` — delimiter `:`, param separator `;`, key/value separator `=`.
- Up to **5 parameters** per command on the serial transport.
- Responses follow the same frame format; multi-value responses send multiple frames.
- Every mutating command that requires a **reboot** to take effect returns `reboot=1` in its ACK.
- Use `C0` (Save Settings) to persist in-memory changes to EEPROM before powering off.
- `255` / `0xFF` means **disabled / not fitted** for any pin field.

---

## System Commands — `F`

These are commands used to configure the system settings and can only be sent from a computer, they are not used for internal communication.

| Command | Example | Purpose |
|---|---|---|
| `F0` — Heart beat | `F0:w=0x00` or `F0:w=0x00;t=1733328600` | Send at rated intervals to monitor connection health. If no ACK received, indicates connection loss. **Params:** `w=<hex_warnings>` - bitmask of active local warnings (hex format). `t=<timestamp>` - Unix timestamp for continuous time sync. PowerControlHub automatically updates clock from `t=` parameter. |
| `F1` — System Initialized | `F1` | Sent by the system when initialization is complete to signal readiness. No params. Used to notify connected devices or software that the control panel is ready for operation. |
| `F2` — Free Memory | `F2` | When received will return the amount of free memory. |
| `F3` — Cpu Usage | `F3` | When received will return the current CPU usage. No params. |
| `F4` — Bluetooth Enabled | `F4` | When received will return the current enabled state of bluetooth 0 off 1 on. No params. |
| `F5` — Wifi Enabled | `F5` | When received will return the current enabled state of wifi 0 off 1 on. No params. |
| `F6` — Set DateTime | `F6:v=1733328600` | Set current date/time. Accepts Unix timestamp (seconds since epoch). Returns current stored time. |
| `F7` — Get DateTime | `F7` | Get current date/time as ISO 8601 datetime string `YYYY-MM-DDTHH:MM:SS`. |
| `F8` — SD Card Present | `F8` | Query if SD card is present. Returns `v=0` (not present) or `v=1` (present). No params. |
| `F9` — SD Card Log File Size | `F9` | Get current log file size in bytes. Returns `v=<bytes>` where bytes is the size of the active log file. Returns `v=0` if no file is open. No params. |
| `F10` — RTC Diagnostic | `F10` | Perform DS1302 RTC diagnostics. Returns status message with RTC health (availability, running state, write protection, time validity). Returns error message if RTC fails any check. No params. |
| `F11` — Uptime | F11 | Returns system uptime as "days HH:MM:SS" (e.g. 2d 03:12:45). No params. |
| `F12` — OTA Check / Apply | `F12` or `F12:apply=1` | Trigger an OTA firmware check against the latest GitHub release. Without params (or `apply=0`) checks only and returns current status. With `apply=1` downloads and applies the update if one is available (or queues apply if a check is already in progress). Response params: `v=<current>` current firmware version, `av=<available>` available version tag (empty if none found yet), `s=<state>` OTA state string. **PCH only** — requires `OTA_AUTO_UPDATE`, ESP32 and WiFi in client mode. |
| `F13` — OTA Status | `F13` | Query current OTA state without triggering a check. Response params: `v=<current>` current firmware version, `av=<available>` available version tag, `s=<state>` OTA state string (`idle`, `checking`, `available`, `downloading`, `rebooting`, `failed`, `uptodate`), `auto=<0\|1>` whether auto-apply is enabled. **PCH only** — requires `OTA_AUTO_UPDATE`, ESP32 and WiFi in client mode. |
| `F14` — PinGuard Mode | `F14` or `F14:a=true` or `F14:a=false;b=false` | Read or write the persistent PinGuard mode flags stored in `SystemHeader::pinGuardFlags`. **Params:** `a=<bool>` — AllowAdvisory: when `true`, advisory (strapping/UART) pins are permitted. `b=<bool>` — Bypass: when `true`, all PinGuard checks are skipped (no validation at all). If both `a` and `b` are `true`, Bypass takes precedence and the system returns `Safe` immediately. Changes are persisted immediately via `saveHeader()` and take effect at runtime without reboot. **Response:** `a=<0\|1>` current AllowAdvisory state, `b=<0\|1>` current Bypass state. No params = read-only. |
| `F15` — Pin Usage | `F15` | Returns a list of all GPIO pins currently assigned in configuration. Pins set to `0xFF` (disabled / not fitted) are omitted. **Serial response:** `v=pin1,pin2,...` (single frame). **WiFi response:** JSON object with a `pins` array. No params. |
| `F16` — Pin Restrictions | `F16` | Returns the compile-time pin restriction table for the current target MCU. Pins are classified as `Hard` (always blocked, e.g. flash-reserved, PSRAM) or `Advisory` (strapping, UART0, USB, JTAG — usable only when PinGuard AllowAdvisory is enabled). **Serial response:** one line per pin as `pin - category` followed by `ACK:F16=ok`. **WiFi response:** JSON object with `"hard"` and `"advisory"` integer arrays. No params. |
| `F17` — Location Types | `F17` | Returns all `LocationType` enum values as descriptors. Each entry has `id` (uint8), `type` (`boat` or `other`), and `desc` (human-readable name). **Serial response:** one line per descriptor as `F17:id=<id>;type=<boat\|other>;desc=<name>` followed by `ACK:F17=ok`. **WiFi response:** JSON object with `"success"`, `"command"`, and `"locations"` array of `{"id", "type", "description"}` objects. No params. |


**OTA behaviour (F12 / F13):**
- Automatic periodic check every 24 hours (first check fires 24 h after boot).
- `F12` without params (or `apply=0`) triggers a check only — it does **not** apply automatically.
- `F12:apply=1` downloads and applies immediately; if a check is in progress the apply is queued.
- Auto-apply is **off by default**; enable via `OtaFlagAutoApply (0x01)` in `SystemHeader::reserved[0]`.
- State transitions broadcast an `F12` automatically as they occur.
- `F13` is read-only — it never triggers any OTA activity.
- OTA is only active when `OTA_AUTO_UPDATE` is defined, the board is an ESP32, `WIFI_SUPPORT` is defined, and WiFi is in connected client mode.

### WiFi system commands
Route: `/api/system/{command}`  
Example: `GET /api/system/F2`

---

## Configuration Commands — `C`

| Command | Example | Purpose |
|---|---|---|
| `C0` — Save Settings | `C0` | Persist current in-memory config to EEPROM. Responds `SAVED` on success. No params. |
| `C1` — Get Settings | `C1` | Request full config dump. PCH replies with multiple frames covering all groups (see below). No params. |
| `C2` — Reset Settings | `C2` | Full reset of all config to factory defaults. No params. |
| `C3` — Rename Location | `C3:Sea Wolf` | Set the location name. Value directly, no `v=` prefix. Empty name → error. |
| `C4` — SPI Pins | `C4:sck=12;mosi=11;miso=13` | Set SD card SPI bus pins. Params: `sck=<pin>;mosi=<pin>;miso=<pin>`. Use `255` to clear any pin. Call `C0` to persist. |
| `C5` — Map Home Button | `C5:1=3` (map) — `C5:1=255` (unmap) | Map a home-page slot (0..3) to a relay (0..7), or `255` to unmap. |
| `C6` — XpdzTone Pin | `C6:v=12` | Set XpdzTone buzzer/piezo pin. `v=255` to disable. Call `C0` to persist. |
| `C7` — Location Type | `C7:v=1` | Set location type: `0`=Motor, `1`=Sail, `2`=Fishing, `3`=Yacht. |
| `C8` — Hw479Rgb Pins | `C8:r=12;g=13;b=14` | Set RGB LED warning indicator pins. Use `255` for any pin not fitted. Call `C0` to persist. |
| `C9` — Sound Start Delay | `C9:v=500` | Delay in milliseconds before sound starts, allowing other processing to complete. |
| `C10` — Bluetooth Enabled | `C10:v=1` | Enable (`1`) / disable (`0`) Bluetooth. Restart required. |
| `C11` — WiFi Enabled | `C11:v=1` | Enable (`1`) / disable (`0`) WiFi. Restart required. |
| `C12` — WiFi Access Mode | `C12:v=1` | Access mode: `0`=AP, `1`=Client. |
| `C13` — WiFi SSID | `C13:RouterName` | Set WiFi SSID (client mode only). Value directly, no `v=` prefix. |
| `C14` — WiFi Password | `C14:Password123` | Set WiFi password. Value directly, no `v=` prefix. |
| `C15` — WiFi Port | `C15:v=80` | Set listen port. Default `80`. Zero or invalid → error. |
| `C16` — WiFi Connection State | `C16` | Returns current `WifiConnectionState` value. Read-only, no params. |
| `C17` — WiFi AP IP Address | `C17:192.168.4.1` | Set AP-mode IP address. Value directly, no `v=` prefix. |
| `C18` — RTC Pins | `C18:dat=4;clk=5;rst=6` | Set DS1302 RTC pins. Use `255` for any pin not fitted. Call `C0` to persist. |
| `C19` — Network Authentication | `C19:e=1;k=MyApiKey;h=MyHmacKey` or `C19:g=1` | Configure WiFi API authentication. **Params:** `e=<0\|1>` enable/disable auth (disabled by default). `k=<key>` set API key (max 31 chars). `h=<key>` set HMAC-SHA256 key (max 31 chars). `g=1` auto-generate device-unique keys from the WiFi MAC address. No params returns current state as `e=<0\|1>;k=<apiKey>;h=<hmacKey>`. Call `C0` to persist. When enabled, all `/api/*` endpoints require either an `X-API-Key` header matching the configured key, or valid `X-Auth-Timestamp` + `X-Auth-Signature` HMAC headers. See [Network Authentication](#network-authentication) below. |
| `C20` — Timezone Offset | `C20:v=-5` | Set UTC timezone offset in hours. Valid range: −12 to +14. |
| `C21` — MMSI | `C21:v=123456789` | Set 9-digit Maritime Mobile Service Identity. Value directly. |
| `C22` — Call Sign | `C22:v=ABCD123` | Set location call sign. Value directly, truncated to max length. |
| `C23` — Home Port | `C23:v=Miami` | Set location home port. Value directly, truncated to max length. |
| `C24` — LED Color | `C24:t=0;c=0;r=255;g=50;b=213` | Set LED RGB color. `t`: `0`=day, `1`=night. `c`: `0`=good, `1`=bad. RGB values 0–255. |
| `C25` — LED Brightness | `C25:t=0;b=75` | Set LED brightness 0–100. `t`: `0`=day, `1`=night. |
| `C26` — LED Auto Switch | `C26:v=1` | Enable/disable automatic day/night LED switching. |
| `C27` — LED Enable States | `C27:g=1;w=1;s=0` | Enable/disable individual LEDs. `g`=GPS, `w`=Warning, `s`=System. |
| `C28` — Control Panel Tones | `C28:t=0;h=400;d=500;p=0;r=30000` | Configure tones. `t`: `0`=good, `1`=bad. `h`=Hz, `d`=duration ms. `p`=preset: `0`=custom, `1`=submarine ping, `2`=double beep, `3`=rising chirp, `4`=descending alert, `5`=nautical bell, `255`=silent. `r`=repeat interval ms (bad only, `0`=no repeat). |
| `C29` — Reload Config from SD | `C29` | **Serial only.** Reload configuration from SD card `config.txt`. |
| `C30` — Export Config to SD | `C30` | **Serial only.** Export current config to SD card. |
| `C31` — SD Card Init Speed | `C31:v=4` | Set SD card SPI init speed in MHz. Valid: 4, 8, 12, 16, 20, 24. Default 4 MHz. Higher speeds should only be used with cards that support high-speed SPI; reduce to 4–8 MHz if experiencing init failures. |
| `C32` — SD Card CS Pin | `C32:v=10` | Set SD card chip-select GPIO pin. Use `255` to disable. Call `C0` to persist. |
| `C33` — Light Sensor Threshold | `C33:v=512` | Analogue threshold (0–1023) above which daytime is detected. Default 512. Uses a rolling average of the last 10 samples; 3 consecutive consistent readings required before day/night state changes (see `S16`). |

**C1 response sequence:**  
PCH sends back: `C3` (name), `C4` (SPI pins), `C5` (home-button mappings), `C7` (location type), `C9` (sound delay), `C10`–`C17` (WiFi/Bluetooth — PCH only), `C18` (RTC pins), `C19` (network auth), `C20`–`C23` (location identity), `C24`–`C27` (LED config), `C28` (tones), `N1`–`N6` (Nextion config), `R6` per relay (names), `R7` per relay (colours), `R8` per relay (default states), `R9` (linked pairs), `R10` (action types), `R11` (pins), `S0` per sensor (sensor config), then `ACK:C1=ok`.

Common errors: `Missing param`, `Empty name`, `Invalid boat type`, `Invalid offset (-12 to +14)`, `MMSI must be 9 digits`, `Invalid speed (4, 8, 12, 16, 20, or 24 MHz)`, `Config not available`, `EEPROM commit failed`.

### WiFi configuration commands
Route: `/api/config/{command}`  
Example: `POST /api/config/C6?v=12`

---

## Nextion Display Configuration Commands — `N`

Each field is its own command so no message exceeds the 5-parameter serial limit.  
**Reboot required** for changes to take effect. Every mutating ACK carries `reboot=1`.  
Serial and network share the same handler — N-commands travel via `/api/config` on the network transport.

| Command | Example | Purpose |
|---|---|---|
| `N0` — Get Nextion Config | `N0` | Returns current config as individual `N1`–`N6` responses. No params. |
| `N1` — Enabled | `N1:v=1` | Enable (`1`) / disable (`0`) the Nextion display. Call `C0` to persist. |
| `N2` — Hardware Serial | `N2:v=1` | Serial mode: `1`=HardwareSerial, `0`=SoftwareSerial (uses rxPin/txPin). Call `C0` to persist. |
| `N3` — RX Pin | `N3:v=16` | Set RX pin. Use `255` to clear. Call `C0` to persist. |
| `N4` — TX Pin | `N4:v=17` | Set TX pin. Use `255` to clear. Call `C0` to persist. |
| `N5` — Baud Rate | `N5:v=9600` | Set serial baud rate. Call `C0` to persist. |
| `N6` — UART Number | `N6:v=1` | ESP32 UART peripheral index. Valid values: `1` or `2` (UART0 reserved for USB/debug). Call `C0` to persist. |

**WiFi route:** `/api/config/{command}`  
**Example:** `POST /api/config/N1?v=1`

**Setup example:**
```
N1:v=1    # Enable Nextion
N2:v=1    # Hardware serial
N3:v=16   # RX pin
N4:v=17   # TX pin
N5:v=9600 # Baud rate
N6:v=1    # UART1
C0        # Save to EEPROM
```

---

## External Sensor Configuration Commands — `E`

These commands configure the `RemoteSensorsConfig` array used to construct `RemoteSensor` instances dynamically at boot. Because each sensor record has more fields than the 5-parameter serial limit, add/update is split across two commands (`E1` for core fields, `E2` for MQTT detail fields).

**Reboot required** for changes to take effect. Every mutating ACK carries `reboot=1`.

| Command | Example | Purpose |
|---|---|---|
| `E0` — Get All | `E0` | Returns two frames per configured sensor (core fields then MQTT fields). No params. |
| `E1` — Set Core Fields | `E1:i=0;id=3;n=Gps;mn=Latitude;ms=latitude` | Set identity and core MQTT fields for sensor at index `i`. Params: `i=<idx>` (0-based, max `ConfigMaxSensors`); `id=<sensorId>` (`SensorIdList` enum value); `n=<name>` (≤20 chars — also used as the serial command ID that external devices push updates to); `mn=<mqttName>` (≤15 chars); `ms=<mqttSlug>` (≤15 chars). Call `C0` to persist. |
| `E2` — Set MQTT Fields | `E2:i=0;mt=latitude;md=;mu=°;bin=0` | Set MQTT type/device-class/unit fields for sensor at index `i`. Params: `i=<idx>`; `mt=<mqttTypeSlug>` (≤15 chars); `md=<mqttDeviceClass>` (≤15 chars, empty to clear); `mu=<mqttUnit>` (≤10 chars, empty to clear); `bin=<0\|1>` (binary sensor flag). Call `C0` to persist. |
| `E3` — Remove | `E3:0` | Remove sensor at index. Remaining entries are compacted down. Call `C0` to persist. |
| `E4` — Rename | `E4:0=GpsUnit` | Rename sensor at index. Param format: `<idx>=<name>`. Call `C0` to persist. |

**`E0` response format (two frames per sensor):**
```
E0:i=<idx>;id=<sensorId>;n=<name>;mn=<mqttName>;ms=<mqttSlug>
E0:i=<idx>;mt=<mqttTypeSlug>;md=<mqttDeviceClass>;mu=<mqttUnit>;bin=<0|1>
```

**`SensorIdList` values for the `id` parameter:**

| Value | Name |
|---|---|
| `0` | WaterSensor |
| `1` | Dht11Sensor |
| `2` | LightSensor |
| `3` | GpsSensor |
| `4` | SystemSensor |
| `5` | BinaryPresenceSensor |
| `6` | VoltageSensor |
| `7` | ReedSensor |

**How external sensors receive value updates:**  
After boot the `name` field doubles as the sensor's serial command ID. An external device pushes readings by sending the name as a command:
```
Gps:lat=51.507351;lon=-0.127758
```
`SensorCommandHandler` routes this to the matching `RemoteSensor` which stores the values for MQTT publishing and status queries.

**WiFi route:** `/api/externalsensor/{command}`  
**GET** `/api/externalsensor/` returns a JSON status block with all configured sensors.  
**Example:** `POST /api/externalsensor/E1?i=0&id=3&n=Gps&mn=Latitude&ms=latitude`

**Setup example (GPS sensor with two channels):**
```
E1:i=0;id=3;n=Gps;mn=Latitude;ms=latitude
E2:i=0;mt=latitude;md=;mu=°;bin=0
E1:i=1;id=3;n=Gps;mn=Longitude;ms=longitude
E2:i=1;mt=longitude;md=;mu=°;bin=0
C0    # Save to EEPROM — reboot to activate
```

Common errors: `Config not available`, `Invalid sensor index`, `Invalid or missing parameters`, `Missing sensor index`, `Missing parameters`, `Missing name`.

---

## Acknowledgement — `ACK`

Every command returns an ACK frame on the serial transport:

| Example | Meaning |
|---|---|
| `ACK:R3=ok` | Command `R3` processed successfully. |
| `ACK:R3=Index out of range` | Command `R3` failed with the given error message. |

WiFi commands return JSON (`{"success":true}` or `{"success":false,"message":"…"}`) rather than ACK frames.

---

## Relay Control & Configuration Commands — `R`

| Command | Example | Purpose |
|---|---|---|
| `R0` — Turn All Off | `R0` | Turn off all relays. No params. |
| `R1` — Turn All On | `R1` | Turn on all relays. No params. |
| `R2` — Retrieve States | `R2` | Return the current state of all relays. |
| `R3` — Set Relay State | `R3:3=1` | Set relay `idx` on (`1`) or off (`0`). Param: `<idx>=<state>`. `idx` 0..7. |
| `R4` — Get Relay State | `R4:3` | Return state of relay at `idx`. Param: `<idx>`. |
| `R5` — Get All Relay Config | `R5` | Returns full relay config as a series of `R6`–`R11` responses. No params. |
| `R6` — Relay Rename | `R6:2=Bilge` or `R6:2=Bilge\|Bilge Pump` | Rename relay `idx`. Param: `<idx>=<shortName>` or `<idx>=<short\|long>`. Short ≤5 chars (home page), long ≤20 chars (buttons page). |
| `R7` — Set Button Color | `R7:2=4` or `R7:2=255` (clear) | Set active (on) button colour for relay `idx`. `0`=Blue, `1`=Green, `2`=Orange, `3`=Purple, `4`=Red, `5`=Yellow, `255`=clear. |

| `R7` — Set Button Color | `R7:2=4` or `R7:2=255` (clear) | Set active (on) button colour for relay `idx`. Accepts Nextion picture IDs (as stored on the device): `2`=Blue, `3`=Green, `4`=Grey, `5`=Orange, `6`=Red, `7`=Yellow, `255`=clear. |
| `R8` — Set Default State | `R8:3=1` | Power-on default state for relay `idx`: `0`=off, `1`=on. |
| `R9` — Link Relays | `R9:3=4` (link) — `R9:3=255` (unlink) | Link two relays so toggling one toggles the other. `255` to unlink. |
| `R10` — Set Action Type | `R10:2=0` / `R10:2=1` / `R10:2=2` | Set relay action type: `0`=Default, `1`=Horn (activates sound system), `2`=NightRelay (activates at night per light sensor). Only one relay per type at a time. |
| `R11` — Set Pin | `R11:2=15` | Set GPIO pin for relay `idx`. Must be a valid non-zero pin number or 255 to disable |

### WiFi relay commands
Route: `/api/relay/{command}`  
Example: `POST /api/relay/R3?3=1`

---

## Sound / Fog-Horn Signal Commands — `H`

| Command | Example | Purpose |
|---|---|---|
| `H0` — Cancel All | `H0` | Cancel all active sound signals immediately. No params. |
| `H1` — Is Active | `H1` | Returns active status as `<SoundType>=<SoundState>` pairs. No params. |
| `H2` — SOS / Danger | `H2` | Activates SOS horn sound (`...---...`) until cancelled. No params. |
| `H3` — Fog Signal | `H3` | Activates fog signal (location-type dependent) every 2 minutes until cancelled. No params. |
| `H4` — Manoeuvre Starboard | `H4` | One short blast. No params. |
| `H5` — Manoeuvre Port | `H5` | Two short blasts. No params. |
| `H6` — Manoeuvre Astern | `H6` | Three short blasts. No params. |
| `H7` — Manoeuvre Danger | `H7` | Five short rapid blasts. No params. |
| `H8` — Overtake Starboard | `H8` | Two prolonged + one short blast. No params. |
| `H9` — Overtake Port | `H9` | Two prolonged + two short blasts. No params. |
| `H10` — Overtake Consent | `H10` | One prolonged + one short + one prolonged + one short blast. No params. |
| `H11` — Overtake Danger | `H11` | Five short rapid blasts. No params. |
| `H12` — Test | `H12` | Test the horn/buzzer. No params. |

### WiFi sound commands
Route: `/api/sound/{command}`  
Example: `POST /api/sound/H3`

---

## Warning Commands — `W`

| Command | Example | Purpose |
|---|---|---|
| `W0` — Warnings Active | `W0:v=3` | Sent by SFB; indicates count of currently active warnings. |
| `W1` — List Warnings | `W1` | Retrieve warning list. Returns one `W2` frame per warning. No params. |
| `W2` — Warning Status | `W2:0x05=1` | Status of a single warning type. Param: `<WarningType>=<0\|1>`. |
| `W3` — Clear Warnings | `W3` | Clear all active warnings. No params. |
| `W4` — Set Warning Status | `W4:0x06=1` | Raise (`1`) or clear (`0`) a specific warning. Param: `<WarningType>=<0\|1>`. |
| `W5` — List Warning Text | `W5` | Returns one `W5:<hex>=<description>` frame per active warning and ends with `ACK:W5=ok`. Use this to get human-readable descriptions of all active warnings. No params. **WiFi response:** `{"success":true,"warnings":["desc1","desc2"]}`. |

### WiFi warning commands
Route: `/api/warning/{command}`  
Example: `GET /api/warning/W1`

---

## MQTT Configuration Commands — `M` (SFB only)

Requires `MQTT_SUPPORT` to be defined. M-commands share the `/api/config` network route.

| Command | Example | Purpose |
|---|---|---|
| `M0` — MQTT Enabled | `M0:v=1` | Enable (`1`) / disable (`0`) MQTT. Query without params returns current state. |
| `M1` — Broker Address | `M1:192.168.1.100` | Set broker hostname or IP. Value directly, no `v=` prefix. |
| `M2` — Broker Port | `M2:v=1883` | Set broker port. Default 1883. |
| `M3` — Username | `M3:myuser` | Set MQTT username. Leave empty for unauthenticated. |
| `M4` — Password | `M4:mypassword` | Set MQTT password. |
| `M5` — Device ID | `M5:boat_seawolf_01` | Set unique device identifier used in all topic paths: `<prefix>/<deviceId>/…`. |
| `M6` — HA Discovery | `M6:v=1` | Enable/disable Home Assistant auto-discovery messages. |
| `M7` — Keep-Alive | `M7:v=60` | PING interval in seconds. Default 60. |
| `M8` — Connection State | `M8` | Returns `v=0` (disconnected) or `v=1` (connected). Read-only. |
| `M9` — Discovery Prefix | `M9:homeassistant` | Base topic prefix. Default `homeassistant`. Used in all discovery and state topics. |

**MQTT topic structure:**
- Relay state / command: `<prefix>/<deviceId>/relay/<idx>/state` / `.../set`
- Sensor data: `<prefix>/<deviceId>/sensor/<type>/state`
- System metrics: `<prefix>/<deviceId>/system/<metric>/state`
- Warnings: `<prefix>/<deviceId>/warning/active/state`
- HA discovery: `<prefix>/switch/<deviceId>/relay<idx>/config`

**Setup example:**
```
M0:v=1              # Enable MQTT
M1:192.168.1.100    # Broker IP
M5:boat_seawolf_01  # Device ID
C0                  # Save to EEPROM
```

### WiFi MQTT commands
Route: `/api/config/{command}` (shares the config route)

---

## Sensor Commands — `S`

`S0`–`S6` are configuration commands (persisted in EEPROM, **reboot required**).  
`S7`–`S24` are live telemetry commands (sensor readings).

### Sensor Configuration — `S0`–`S6`

| Command | Example | Purpose |
|---|---|---|
| `S0` — Get All Sensor Config | `S0` or `S0?meta=1` | Returns one `S0` frame per configured sensor: `i=<idx>;t=<type>;n=<name>;p0=<pin>;p1=<pin>;u0=<opt1_0>;u1=<opt1_1>;o0=<opt2_0>;o1=<opt2_1>;en=<0\|1>`. The labels in the response are read-only aliases: `p0`/`p1` = `pins[0]`/`pins[1]`; `u0`/`u1` = `options1[0]`/`options1[1]` (signed byte); `o0`/`o1` = `options2[0]`/`options2[1]` (signed 16-bit). **WiFi only:** Append `?meta=1` to include a `"meta"` block with sensor type descriptors (field labels, types, ranges, defaults) for self-describing dynamic UI. No serial params. |
| `S1` — Add / Update Sensor | `S1:i=<idx>;t=<type>;o0=<val>;o1=<val>` | Add or update sensor at index `i`. **Only four params are accepted:** `i` (sensor index), `t` (type — `SensorIdList` enum value), `o0` (writes `options1[0]`), `o1` (writes `options1[1]`). Pins and all other options must be set separately via `S4` and `S6` **after** `S1`. All four params are required. ACK carries `reboot=1`. |
| `S2` — Remove Sensor | `S2:v=0` | Remove (clear) sensor at index. Param: `v=<idx>`. ACK carries `reboot=1`. |
| `S3` — Rename Sensor | `S3:0=Bilge Pump` | Rename sensor at index. Param: `<idx>=<name>`. ACK carries `reboot=1`. |
| `S4` — Set Sensor Pin | `S4:i=0;s=0;v=34` | Set a GPIO pin for sensor `i`. **All three params are required:** `i` (sensor index), `s` (pin slot, 0..`ConfigMaxSensorPins-1`), `v` (GPIO pin number; `255` to clear). Each sensor type uses specific slots — see the sensor type table below. ACK carries `reboot=1`. |
| `S5` — Set Sensor Enabled | `S5:0=1` | Enable (`1`) / disable (`0`) sensor at index. ACK carries `reboot=1`. |
| `S6` — Set Sensor Option | `S6:i=0;s=0;o=0;v=1` | Set an option value for sensor `i`. **All four params are required:** `i` (sensor index), `s` (slot, 0 or 1), `o` (group: `0`=`options1` signed byte, `1`=`options2` signed 16-bit), `v` (value). See the sensor type table below for which slot/group each sensor uses. ACK carries `reboot=1`. |

> **Understanding the field storage and response labels**
>
> Sensor configuration is stored in three arrays inside `SensorEntry`. The `S0` response exposes these with short labels for readability, but you **cannot use those labels as input to `S1`**:
>
> | `S0` response label | Underlying storage | Command to write it |
> |---|---|---|
> | `p0` | `pins[0]` | `S4:i=<idx>;s=0;v=<pin>` |
> | `p1` | `pins[1]` | `S4:i=<idx>;s=1;v=<pin>` |
> | `u0` | `options1[0]` (signed byte) | `S6:i=<idx>;s=0;o=0;v=<val>` |
> | `u1` | `options1[1]` (signed byte) | `S6:i=<idx>;s=1;o=0;v=<val>` |
> | `o0` | `options2[0]` (signed 16-bit) | `S6:i=<idx>;s=0;o=1;v=<val>` |
> | `o1` | `options2[1]` (signed 16-bit) | `S6:i=<idx>;s=1;o=1;v=<val>` |
>
> `S1` only writes `options1[0]` and `options1[1]` (its `o0`/`o1` params). Everything else requires a separate `S4` or `S6` call.

**Sensor type values (`t` for `S1`) — field mapping:**

The table below shows what each storage field means for each sensor type and which command sets it.

| Value | Name | Field | Meaning | Command |
|---|---|---|---|---|
| `0` | Water Sensor | `pins[0]` | Data pin | `S4:i=<idx>;s=0;v=<pin>` |
| | | `pins[1]` | Power pin | `S4:i=<idx>;s=1;v=<pin>` |
| `1` | DHT11 | `pins[0]` | Data pin | `S4:i=<idx>;s=0;v=<pin>` |
| `2` | Light Sensor | `pins[0]` | Analogue pin | `S4:i=<idx>;s=0;v=<pin>` |
| | | `options1[0]` | `1`=digital mode, `0`=analogue mode | `S1:…;o0=<val>;o1=0` |
| `3` | GPS Sensor | `pins[0]` | RX pin (**required** — `255` raises `GpsInvalidConfig`) | `S4:i=<idx>;s=0;v=<pin>` |
| | | `pins[1]` | TX pin (**required** — `255` raises `GpsInvalidConfig`) | `S4:i=<idx>;s=1;v=<pin>` |
| | | `options1[0]` | UART number: `1` or `2` (ESP32 only; other values default to `2`). Baud rate fixed at 9600. | `S6:i=<idx>;s=0;o=0;v=<uart>` |
| `4` | System Sensor | — | No pin or option required | — |
| `5` | Binary Presence Sensor | `pins[0]` | Sensor pin | `S4:i=<idx>;s=0;v=<pin>` |
| | | `pins[1]` | onDetected payload | `S4:i=<idx>;s=1;v=<val>` |
| | | `pins[2]` | onClear payload | `S4:i=<idx>;s=2;v=<val>` |
| | | `options1[0]` | Active state: `1`=HIGH, `0`=LOW | `S1:…;o0=<val>;o1=0` |
| | | `options1[1]` | onDetected action | `S6:i=<idx>;s=1;o=0;v=<val>` |
| | | `options2[1]` | onClear action | `S6:i=<idx>;s=1;o=1;v=<val>` |
| `6` | Voltage Sensor | `pins[0]` | Analogue input pin | `S4:i=<idx>;s=0;v=<pin>` |
| | | `options1[0]` | ADC Vref in tenths of a volt (`50`=5.0 V, `33`=3.3 V; `0`=default 5.0 V) | `S6:i=<idx>;s=0;o=0;v=<val>` |
| | | `options1[1]` | R2 in tenths of kΩ (`75`=7.5 kΩ; `0`=default 7.5 kΩ) | `S6:i=<idx>;s=1;o=0;v=<val>` |
| | | `options2[0]` | R1 in whole kΩ (`30`=30 kΩ; `0`=default 30 kΩ) | `S6:i=<idx>;s=0;o=1;v=<val>` |
| | | `options2[1]` | Low-voltage warning threshold in tenths of a volt (`114`=11.4 V; `0`=disabled) | `S6:i=<idx>;s=1;o=1;v=<val>` |
| `7` | Reed Sensor | `pins[0]` | Sensor pin | `S4:i=<idx>;s=0;v=<pin>` |
| | | `pins[1]` | Active output pin (driven HIGH while contact active; `255`=disabled) | `S4:i=<idx>;s=1;v=<pin>` |
| | | `options1[0]` | Contact type: `0`=normally open (NO), `1`=normally closed (NC) | `S1:…;o0=<val>;o1=0` |

**Voltage sensor setup example (30 kΩ / 7.5 kΩ divider, 12 V system, warn below 11.4 V):**
```
S1:i=1;t=6;o0=0;o1=0   # Add Voltage sensor (type 6)
S3:1=Battery            # Name it
S4:i=1;s=0;v=34         # Analogue pin = 34
S6:i=1;s=0;o=0;v=50     # Vref = 5.0 V (50 tenths)
S6:i=1;s=1;o=0;v=75     # R2 = 7.5 kΩ (75 tenths)
S6:i=1;s=0;o=1;v=30     # R1 = 30 kΩ
S6:i=1;s=1;o=1;v=114    # Low-voltage warn at 11.4 V (114 tenths); use 0 to disable
S5:1=1                  # Enable sensor
C0                      # Save to EEPROM — reboot to activate
```

Common sensor config errors: `Config not available`, `Invalid sensor index`, `Missing sensor index`, `Missing parameters`, `Missing name`, `Invalid pin slot`, `Invalid option slot`.

**GPS setup example (ESP32 NodeMCU-32S — Nextion on UART2, GPS remapped to UART1 on pins 25/26):**
```
S1:i=0;t=3;o0=0;o1=0   # Add GPS sensor (type 3)
S3:0=GPS                # Name it
S4:i=0;s=0;v=25         # RX pin = 25
S4:i=0;s=1;v=26         # TX pin = 26
S6:i=0;s=0;o=0;v=1      # UART number = 1 (avoids conflict with Nextion on UART2)
S5:0=1                  # Enable sensor
C0                      # Save to EEPROM — reboot to activate
```
> **Note:** If either RX or TX pin is `255` (disabled) the GPS sensor will **not** be created at boot and a `GpsInvalidConfig` warning will be raised. Both pins must be set before saving.

### Sensor Telemetry — `S7`–`S24`

| Command | Example | Purpose |
|---|---|---|
| `S7` — Temperature | `S7:v=72.5` | Temperature reading. |
| `S8` — Humidity | `S8:v=55.2` | Humidity reading. |
| `S9` — Bearing | `S9:v=128` | Compass bearing in degrees. |
| `S10` — Direction | `S10:v=NNW` | Cardinal direction string. |
| `S11` — Speed | `S11:v=3.4` | Speed reading. |
| `S12` — Compass Temp | `S12:v=23.4` | Compass module temperature. |
| `S13` — Water Level | `S13:v=3.4` | Water level reading. |
| `S14` — Water Pump Active | `S14:v=1` | Water pump state: `0`=off, `1`=on. |
| `S15` — Horn Active | `S15:v=1` | Horn state: `0`=off, `1`=on. |
| `S16` — Light Sensor | `S16:v=1` | Daytime state: `0`=night, `1`=day. See `C33` for threshold. |
| `S17` — GPS Lat/Long | `S17:lat=51.507351;lon=-0.127758` | GPS coordinates in decimal degrees (6 d.p.). |
| `S18` — GPS Altitude | `S18:v=125.50` | Altitude in metres (2 d.p.). |
| `S19` — GPS Speed | `S19:v=15.75;course=245.30;dir=NNW` | Speed km/h, course degrees (0–360), optional cardinal direction. |
| `S20` — GPS Satellites | `S20:v=8` | Number of connected satellites. |
| `S21` — GPS Distance | `S21:v=1.23` | Total distance travelled. |
| `S22` — Binary Presence | `S22:v=1;name=PIR` | Pin-change event: `v=1` detected, `v=0` clear. `name` = configured sensor name. |
| `S23` — Voltage | `S23:v=12.45;avg=12.41` | Voltage reading from the DC 0–25 V module. `v`=latest sample in volts (2 d.p.), `avg`=10-reading rolling average in volts (2 d.p.). Raises `LowVoltage` warning when `avg` drops below the configured threshold; clears when it recovers. |
| `S24` — Reed | `S24:v=1;name=Reed` | Contact state-change event: `v=1` active (detected), `v=0` clear. Sent only when the contact changes state. `name` = configured sensor name. |

### WiFi sensor commands
Route: `/api/sensor/`  
Example: `GET /api/sensor/`

---

## Timer / Scheduler Commands — `T`

Up to 20 scheduled events stored in EEPROM (6 max on platforms with < 1 KB EEPROM). Each event has a **trigger**, an optional **condition**, and an **action**.

| Command | Example | Purpose |
|---|---|---|
| `T0` — List Events | `T0` | Returns `count=<n>;s=<0,1,0,…>` — enabled state for every slot. |
| `T1` — Get Event | `T1:v=2` | Returns full detail of event at index `v` in `T2` format. Error if slot empty. |
| `T2` — Set Event | `T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0` | Add/update event at index `i`. Params: `i=<idx>;e=<enabled>;t=<type,b0,b1,b2,b3>;c=<type,b0,b1,b2,b3>;a=<type,b0,b1,b2,b3>`. Each field is 5 comma-separated values (type + 4 payload bytes). Call `C0` to persist. |
| `T3` — Delete Event | `T3:v=2` | Clear event at index `v`. Call `C0` to persist. |
| `T4` — Enable / Disable | `T4:i=2;v=1` | Enable (`v=1`) or disable (`v=0`) event at index `i`. Call `C0` to persist. |
| `T5` — Clear All | `T5` | Remove all 20 events. Call `C0` to persist. |
| `T6` — Trigger Now | `T6:v=2` | Execute the action of event `v` immediately, bypassing trigger and condition. Useful for testing. |

**Trigger types (`t` first value):**

| Type | Name | Payload `b0..b3` |
|---|---|---|
| `0` | None | Never fires automatically |
| `1` | TimeOfDay | `b0`=hour (0–23), `b1`=minute (0–59) |
| `2` | Sunrise | `b0..b1`=int16 offset minutes (+after, −before) |
| `3` | Sunset | `b0..b1`=int16 offset minutes |
| `4` | Interval | `b0..b1`=uint16 interval minutes |
| `5` | DayOfWeek | `b0`=bitmask (bit0=Mon…bit6=Sun). Mon–Fri=31, weekends=96, all days=127 |
| `6` | Date | `b0`=day (1–31), `b1`=month (1–12) |

**Condition types (`c` first value):**

| Type | Name | Payload `b0..b3` |
|---|---|---|
| `0` | None | Always fires |
| `1` | TimeWindow | `b0`=start_hour, `b1`=start_min, `b2`=end_hour, `b3`=end_min |
| `2` | DayOfWeek | `b0`=bitmask (same layout as trigger DayOfWeek) |
| `3` | IsDark | True between sunset and sunrise |
| `4` | IsDaylight | True between sunrise and sunset |
| `5` | RelayIsOn | `b0`=relay index |
| `6` | RelayIsOff | `b0`=relay index |

**Action types (`a` first value):**

| Type | Name | Payload `b0..b3` |
|---|---|---|
| `0` | None | Nothing happens |
| `1` | RelayOn | `b0`=relay index |
| `2` | RelayOff | `b0`=relay index |
| `3` | RelayToggle | `b0`=relay index |
| `4` | RelayPulse | `b0`=relay index, `b1..b2`=uint16 duration seconds (little-endian) |
| `5` | AllRelaysOn | — |
| `6` | AllRelaysOff | — |
| `7` | SetPinHigh | `b0`=GPIO pin number |
| `8` | SetPinLow | `b0`=GPIO pin number |

**Examples:**
```
# At 18:30 turn Relay 3 off (relay index 2, 0-based)
T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0
C0

# 10 minutes after sunset turn Relay 2 on (relay index 1, offset +10 min)
T2:i=1;e=1;t=3,10,0,0,0;c=0,0,0,0,0;a=1,1,0,0,0
C0

# At sunrise on weekdays only, toggle Relay 1 (relay index 0, Mon–Fri bitmask=31)
T2:i=2;e=1;t=2,0,0,0,0;c=2,31,0,0,0;a=3,0,0,0,0
C0

# Every 30 minutes, pulse Relay 5 for 10 seconds (relay index 4, interval=30, duration=10)
T2:i=3;e=1;t=4,30,0,0,0;c=0,0,0,0,0;a=4,4,10,0,0
C0
```

Common errors: `Index out of range`, `Missing params`, `Invalid trigger type`, `Invalid condition type`, `Invalid action type`, `Invalid enabled value (0 or 1)`, `Slot is empty`.

### WiFi timer commands
Route: `/api/timer/{command}`  
Example: `GET /api/timer/T0`

---

## Network Authentication

When network auth is enabled (`C19:e=1`), every request to `/api/*` endpoints must include authentication credentials. Two authentication methods are supported; clients may use either one.

### API Key (simple)

Include the `X-API-Key` header with the value of the configured API key:

```bash
curl -H "X-API-Key: ak-sfb-A1B2C3" http://DEVICE/api/system/F7
```

### HMAC-SHA256 (strong)

Include two headers: `X-Auth-Timestamp` (Unix epoch seconds) and `X-Auth-Signature` (lowercase hex HMAC-SHA256 of the canonical request).

**Canonical signing input format:**
```
<timestamp>\n<METHOD>\n<path>\n<body>
```

- `timestamp` — same value as `X-Auth-Timestamp`
- `METHOD` — HTTP method: `GET`, `POST`, etc.
- `path` — request path, e.g. `/api/relay/R3`
- `body` — raw request body for POST; empty string for GET

**Example (Bash):**
```bash
TS=$(date +%s)
PATH="/api/system/F7"
METHOD="GET"
BODY=""
HMAC_KEY="hk-sfb-A1B2C3"

SIGN_INPUT="${TS}\n${METHOD}\n${PATH}\n${BODY}"
SIG=$(echo -ne "$SIGN_INPUT" | openssl dgst -sha256 -hmac "$HMAC_KEY" | awk '{print $NF}')

curl -H "X-Auth-Timestamp: ${TS}" -H "X-Auth-Signature: ${SIG}" http://DEVICE${PATH}
```

### Timestamp window and clock bootstrap

- The HMAC timestamp must be within ±300 seconds (5 minutes) of the device's clock.
- **`F6` (Set DateTime) is always permitted via all transports**, even when auth is enabled, as long as the device clock has **never** been synchronized. Once time is set, `F6` requires authentication like all other commands. This prevents a deadlock where the client needs auth to set the clock but the clock is wrong, causing HMAC verification to fail.
- Clients can use an **API Key** to bootstrap the clock via `F6`, then switch to HMAC for stronger ongoing security.

### Best practices

1. **Enable auth after initial setup:** `C19:g=1` to generate per-device unique keys, then `C19:e=1` to enable.
2. **Persist changes:** Call `C0` (Save Settings) after configuring auth to survive power cycles.
3. **Synchronize the clock:** Use `F0:w=0x00;t=<timestamp>` heartbeat or `F6:v=<timestamp>` to keep the device clock accurate for HMAC.
4. **Rotate keys periodically:** `C19:k=<newkey>` and `C19:h=<newkey>` then `C0` to persist.

Common errors: `API key too long`, `HMAC key too long`.

---

## Notes

- All commands and parameters are case-sensitive.
- Commands must be sent in the exact format shown, including delimiters.
- All WiFi responses are JSON formatted.
- Changes to sensor config (`S`), external sensor config (`E`), and Nextion display config (`N`) require a **reboot** to take effect.
- Always call `C0` (Save Settings) after making config changes you want to survive a power cycle.

