# Communication Commands
The following sections indicate the types of commands that can be sent to the Boat Control Panel, with examples and purpose.

## System Commands
These are commands used to configure the system settings and can only be sent from a computer, they are not used for internal communication.

| Command | Example | Purpose |
|---|---|---|
| `F0` — Heart beat | `F0:w=0x00` (SFB) or `F0:w=0x00;t=1733328600` (BCP) | Send at rated intervals to monitor connection health. If no ACK received, indicates connection loss. **Params:** `w=<hex_warnings>` - bitmask of active local warnings (hex format). `t=<timestamp>` - (BCP only) Unix timestamp for continuous time sync. Smart Fuse Box automatically updates clock from `t=` parameter. |
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

**Heartbeat Behavior:**
- Sent every 1 second (1000ms) from both devices
- Boat Control Panel (BCP) includes time sync: `F0:w=0x00;t=1733328600`
- Smart Fuse Box (SFB) includes only warnings: `F0:w=0x00`
- Connection timeout occurs after 3 seconds without response
- Time synchronization is automatic - no separate F6 commands needed during normal operation
- F6 command can still be used for manual time updates or initial configuration
- 
### Wifi System Commands (SFB)
Only F0 is supported via wifi.
Route: /api/system/{command}
Example: Return free memory = /api/system/F2 


------


## Configuration Commands
These are commands used to configure the system settings and can only be sent from a computer, they are not used for internal communication.

| Command | Example | Purpose |
|---|---|---|
| `C0` — Save settings | `C0` | Persist current in-memory config to EEPROM. Responds `SAVED` on success; error `EEPROM commit failed` on failure. No params. |
| `C1` — Get settings | `C1` | Request full config. Device replies with multiple commands: `C3 <boatName>`, `C4 <idx>:<shortName|longName>` for each relay, `C5 <slot>:<relay>` for each home-slot mapping, `C6 <idx>=<color>` for each button color, `C7 v=<type>` for vessel type, `C8 v=<relay>` for sound relay, `C9 v=<delay>` for sound delay, `C10`-`C17` for WiFi/Bluetooth settings (Arduino Uno R4 only), `C18 <relay>=<state>` for default relay states, `C19 <relay>=<linkedrelay>` for linked relays, `C20 v=<offset>` for timezone, `C21 <mmsi>` for MMSI, `C22 <callsign>` for call sign, `C23 <homeport>` for home port, then `OK`. No params. |
| `C2` — Reset settings | `C2` | Request full reset of all config settings. No params. |
| `C3` — Rename boat (BCP) | `C3:Sea Wolf` or `C3:name=SeaWolf` | Set the boat name. `<key>:<value>` pair (value is used as boat name. Empty name → error. Name is truncated to configured max length. |
| `C4` — Rename relay (BCP) | `C4:2=Bilge` or `C4:2=Bilge\|Bilge Pump` | Rename a relay. Param format: `<idx>=<shortName>` or `<idx>=<shortName|longName>`. `idx` must be 0..7 (`RELAY_COUNT`). If no pipe character or long name provided, the short name is used for both. Short name is truncated to 5 chars (used on home page), long name is truncated to 20 chars (used on buttons page). Missing name → error. |
| `C5` — Map home button (BCP) | `C5:1=3` (map) — `C5:1=255` (unmap) | Map a home-page slot to a relay. Param format: `<slot>:<relay>`. `button` must be 0..3 (`HOME_BUTTONS`). `relay` must be 0..7 or `255` to clear/unmap. |
| `C6` — Map home button color (BCP) | `C6:0=4` (map button 1 to Red when activated) — `C6:1=255` (unmap colors) | Map a home-page button to a color when activated (on). Param format: `<slot>:<relay>`. `slot` must be 0..3 (`ConfigManager::HOME_SLOTS`). `relay` must be 0..7 or `255` to clear/unmap. |
| `C7` — Set vessel type | `C7:v=1` | Set the vessel type. Param format: `v=<type>`. Possible values for `<type>` are: 0 (Motor), 1 (Sail), 2 (Fishing), 3 (Yacht). Uses enum values as defined in `Config.h`. Invalid or missing value → error. |
| `C8` — Sound relay button | `C8:v=3` (map) — `C8:v=255` (unmap) | Map the sound system (horn) to a relay. Param format: `v=<relay>`. `relay` must be 0..7 or `255` to clear/unmap. Auto-renames relay to "Sound" / "Sound\r\nSignals" if not already named. |
| `C9` — Sound delay Start | `C9:v=0xFF` | Sets the delay before the sound is started in milliseconds, allows other processing to continue so as sounds are not cut off. Invalid or missing value → error. |
| `C10` — Bluetooth Enabled (SFB) | `C10:v=1` | Sets the enabled state of bluetooth, 0 disabled, 1 enabled, if disabling then a restart is required. Invalid or missing value → error. |
| `C11` — Wifi Enabled (SFB) | `C11:v=1` | Sets the enabled state of wifi, 0 disabled, 1 enabled, if disabling then a restart is required. Invalid or missing value → error. |
| `C12` — Wifi Access Mode (SFB) | `C12:v=1` | Sets access mode, 0 is AP and 1 is client |
| `C13` — Wifi SSID (SFB) | `C13:RouterName` | Set's the wifi SSID, only available if access mode is client. Param format: SSID value directly (no v= prefix). |
| `C14` — Wifi Password (SFB) | `C14:Password123` | Set's the wifi password, only available if access mode is client. Param format: password value directly (no v= prefix). |
| `C15` — Wifi Port (SFB) | `C15:v=80` | Set's the wifi port, this is the port used to listen on, default value is 80. Invalid or zero port → error. |
| `C16` — Wifi connection state (SFB) | `C16` | Wifi connection state, no params, returns WifiConnectionState value. Note: Not implemented in ConfigCommandHandler - use SystemCommandHandler. |
| `C17` — Wifi AP Ip address (SFB) | `C17:192.168.4.1` | Sets the IP address when using access point mode, default is 192.168.4.1. Param format: IP address directly (no v= prefix). |
| `C18` — Initial relay on | `C18:3=1` — `C18:3=0`  | Set's the initial status of a relay to on by default. Param format: `<relay>=<value>`. `relay` must be 0..7 (`RELAY_COUNT`), `value` must be 0 (off by default) or 1 (on by default). |
| `C19` — Link Relays | `C19:3=4` — `C19:3=255` (unlink) | Links relays together, if one relay is switched on, the linked relay is switched on, and vice versa for switching off. Param format: `<relay>=<linkedrelay>`. `relay` must be 0..7 (`RELAY_COUNT`) `linkedrelay` must be 0..7 or `0xFF` (255) to unlink. |
| `C20` — Timezone Offset | `C20:v=-5` or `C20:v=8` | Set timezone offset from UTC in hours. Param format: `v=<offset>`. Valid range: -12 to +14. Invalid offset → error. |
| `C21` — MMSI | `C21:123456789` | Set Maritime Mobile Service Identity (MMSI) number. Param format: MMSI value directly (no v= prefix). Must be exactly 9 digits. Invalid length → error. |
| `C22` — Call Sign | `C22:ABCD123` | Set vessel call sign. Param format: call sign value directly (no v= prefix). Truncated to configured max length (ConfigCallSignLength). |
| `C23` — Home Port | `C23:Miami` | Set vessel home port. Param format: port name directly (no v= prefix). Truncated to configured max length (ConfigHomePortLength). |
| `C24` — LED Color | `C24:t=0;c=0;r=255;g=50;b=213` | Set LED RGB color. Param format: `t=<type>;c=<colorset>;r=<red>;g=<green>;b=<blue>`. `type` must be 0 (day) or 1 (night). `colorset` must be 0 (good) or 1 (bad). RGB values 0-255. |
| `C25` — LED Brightness | `C25:t=0;b=75` | Set LED brightness. Param format: `t=<type>;b=<brightness>`. `type` must be 0 (day) or 1 (night). `brightness` must be 0-100. |
| `C26` — LED Auto Switch | `C26:v=true` or `C26:v=1` | Enable/disable auto day/night switching. Param format: `v=<value>`. `value` must be true/false or 1/0. |
| `C27` — LED Enable States | `C27:g=true;w=true;s=false` | Enable/disable individual LEDs. Param format: `g=<gps>;w=<warning>;s=<system>`. Each value must be true/false or 1/0. `g`=GPS LED, `w`=Warning LED, `s`=System LED. |
| `C28` — Control Panel Tones | `C28:t=0;h=400;d=500;p=0;r=30000` | Configure control panel tones, 2 categories, good and bad, good is only used at startup to indicate all is working, bad is used in response to warnings. t=0 or 1 (0 good or 1 bad), h = tone Hz, d = duration in milliseconds, p = preset 0 custom tone/duration, 1 = submaring ping, 2 = double beep, 3 = rising chirp, 4 = descending alert, 5 = nautical bell, 0xFF = no sound.  r = repeat interval in milliseconds where 0 is do not repeat, this is only used for bad sounds. |
| `C29` — Reload Config from SD (SFB) | `C29` | **Local only (Serial/USB)**. Reload configuration from SD card config.txt file... |
| `C30` — Export Config to SD (SFB) | `C30` | **Local only (Serial/USB)**. Export current in-memory configuration to SD card... |
| `C31` — SD Card Initialize Speed (SFB) | `C31:v=4` | Set SD card SPI initialization speed in MHz. Param format: `v=<speed>`. Valid values: 4, 8, 12, 16, 20, 24. Default is 4 MHz. **Note:** Higher speeds (16+ MHz) should only be used with high-quality SD cards that explicitly support high-speed SPI mode. Using speeds that are too high for your SD card may result in initialization failures or data corruption. If experiencing SD card issues, try reducing the speed to 4 or 8 MHz. |

### MQTT Configuration Commands (SFB)
These commands configure MQTT connectivity for Home Assistant and other MQTT-based integrations.

| Command | Example | Purpose |
|---|---|---|
| `M0` — MQTT Enabled | `M0:v=1` | Enable/disable MQTT. Param format: `v=<value>`. `value` must be 0 (disabled) or 1 (enabled). Query without params returns current state. |
| `M1` — MQTT Broker | `M1:192.168.1.100` | Set MQTT broker address. Param format: broker address directly (no v= prefix). Supports IP addresses or hostnames. Query without params returns current broker. |
| `M2` — MQTT Port | `M2:v=1883` | Set MQTT broker port. Param format: `v=<port>`. Default is 1883. Query without params returns current port. |
| `M3` — MQTT Username | `M3:myuser` | Set MQTT username for authentication. Param format: username directly (no v= prefix). Optional - leave empty for no authentication. Query returns "Username" or empty string if no username has been set. |
| `M4` — MQTT Password | `M4:mypassword` | Set MQTT password for authentication. Param format: password directly (no v= prefix). Optional - leave empty for no authentication. Query returns "Password" or empty string if no password has been setM4. |
| `M5` — MQTT Device ID | `M5:boat_seawolf_01` | Set unique MQTT device identifier. Param format: device ID directly (no v= prefix). Used in topic paths: `smartfusebox/<deviceId>/...`. Query without params returns current device ID. |
| `M6` — HA Discovery | `M6:v=1` | Enable/disable Home Assistant auto-discovery. Param format: `v=<value>`. `value` must be 0 (disabled) or 1 (enabled). When enabled, publishes discovery messages for automatic entity configuration. Query without params returns current state. |
| `M7` — Keep Alive Interval | `M7:v=60` | Set MQTT keep-alive interval in seconds. Param format: `v=<seconds>`. Default is 60 seconds. Controls PING frequency. Query without params returns current interval. |
| `M8` — MQTT Connection State | `M8` | Query MQTT connection state. Returns `v=0` (disconnected/disabled) or `v=1` (connected). Read-only command, no parameters accepted. |
| `M9` — Discovery Prefix | `M9:homeassistant` | Set Home Assistant discovery prefix. Also used as the base topic prefix for all MQTT messages. Param format: prefix directly (no v= prefix). Default is "homeassistant". This prefix is used in discovery topic paths: `<prefix>/switch/<deviceId>/relay<index>/config` and state/command topics: `<prefix>/<deviceId>/relay/<index>/state`. Query without params returns current prefix. |

**MQTT Topic Structure:**
- Relay control: `<discoveryPrefix>/<deviceId>/relay/<index>/set` (command), `<discoveryPrefix>/<deviceId>/relay/<index>/state` (state)
- Sensor data: `<discoveryPrefix>/<deviceId>/sensor/<type>/state`
- System info: `<discoveryPrefix>/<deviceId>/system/<metric>/state`
- Warnings: `<discoveryPrefix>/<deviceId>/warning/active/state`
- Discovery: `<discoveryPrefix>/switch/<deviceId>/relay<index>/config`

**Example with default prefix (homeassistant):**
- State topic: `homeassistant/smartfusebox_01/relay/0/state`
- Command topic: `homeassistant/smartfusebox_01/relay/0/set`
- Discovery topic: `homeassistant/switch/smartfusebox_01/relay0/config`

**Example with custom prefix (myboat):**
- State topic: `myboat/boat_seawolf/relay/0/state`
- Command topic: `myboat/boat_seawolf/relay/0/set`
- Discovery topic: `myboat/switch/boat_seawolf/relay0/config`

**Setup Example:**
```
M0:v=1                          # Enable MQTT
M1:192.168.1.100                # Set broker IP
M5:boat_seawolf_01              # Set device ID
C0                              # Save to EEPROM
```

Common MQTT error responses: `Config not available`, `Invalid port`, `Invalid keep-alive interval`, `Invalid value (true/false or 0/1)`.


Common error responses you may see: `Missing param`, `Missing params`, `Missing name`, `Empty name`, `Index out of range`, `Button out of range`, `Slot out of range`, `Relay out of range (or 255 to clear)`, `Invalid value (0 or 1)`, `Invalid mode (0=AP, 1=Client)`, `Only available in Client mode`, `Invalid port`, `Invalid offset (-12 to +14)`, `MMSI must be 9 digits`, `Invalid boat type`, `No available link slots`, `EEPROM commit failed`, `Unknown config command`, `Invalid type (0=day, 1=night)`, `Brightness must be 0-100`, `Invalid value (true/false or 0/1)`, `Missing params (t,r,g,b)`, `Missing params (t,b)`, `Missing params (g,w,s)`, `Invalid speed (4, 8, 12, 16, 20, or 24 MHz)`, `Config not available`, `Invalid port`, `Invalid keep-alive interval`.


### Wifi Configuration Commands (SFB)
Route: /api/config/{command}
Example: Specify sound relay button = /api/config/C8?v=3 


------


## Acknowledgement Commands
These commands are used in response to receiving a command.

| Command | Example | Purpose |
|---|---|---|
| `ACK` — Acknowledgement | `ACK:C4=Index out of range` | Indicates that the C4 command was processed and the index specified was out of range. |
| `ACK` — Acknowledgement | `ACK:C4=ok` | Indicates that the C4 command was processed successfully. |

### Wifi Acknowledgement Commands 
Not supported

------


## Relay Control Commands (SFB)
These commands are used to control the relays on the Boat Control Panel. Commands can be sent from a computer or generated internally by the Boat Control Panel.

| Command | Example | Purpose |
|---|---|---|
| `R0` — Turn All Off | `R0` | Indicates that the C4 command was processed and the index specified was out of range. |
| `R1` — Turn All On | `R1` | Indicates that the C4 command was processed successfully. |
| `R2` — Retrieve States | `R2` | Retrieve the state of all relays. |
| `R3` — Relay State Set | `R3:3=1` (turn on relay 3) — `R3:5=0` (turn off relay 5) | Set the state of a specific relay. Param format: `<idx>=<state>`. `idx` must be 0..7 (`RELAY_COUNT`). `state` must be `0` (off) or `1` (on). |
| `R4` — Relay State Get | `R4:3` (retrieves status of relay 3) — `R4:5` (returns status of relay 5). Param format: `<idx>`. `idx` must be 0..7 (`RELAY_COUNT`). |


### Wifi Relay Commands (SFB)
Route: /api/relay/{command}
Example: Set relay state to on = /api/relay/R3?3=1
Returns JSON formatted response with all relay states.

------


## Sensor Commands
These commands are used to send sensor data from the Boat Control Panel to a computer.

| Command | Example | Purpose |
|---|---|---|
| `S0` — Temperature | `S0:v=72.5` | Send temperature sensor data. Param format: `<sensor>=<value>`. |
| `S1` — Humidity | `S1:v=55.2` | Send humidity sensor data. Param format: `<sensor>=<value>`. |
| `S2` — Bearing | `S2:v=128` | Send bearing sensor data. Param format: `<sensor>=<value>`. |
| `S3` — Direction | `S3:v=NNW` | Send direction sensor data. Param format: `<sensor>=<value>`. |
| `S4` — Speed | `S4:v=3.4` | Send speed sensor data. Param format: `<sensor>=<value>`. |
| `S5` — Compass Temp | `S5:v=23.4` | Send compass temperature sensor data. Param format: `<sensor>=<value>`. |
| `S6` — Water Level | `S6:v=3.4` | Send water level sensor data. Param format: `<sensor>=<value>`. |
| `S7` — Water Pump Active  | `S7:v=1` | Send water pump active status. Param format: `<sensor>=<value>`, 0 = off, 1 = on. |
| `S8` — Horn Active | `S8:v=1` | Send horn active status. Param format: `<sensor>=<value>`, 0 = off, 1 = on. |
| `S9` — Light Sensor Active  | `S9:v=1` | Send light sensor daytime value. Param format: `<sensor>=<value>`, 0 = off (night time), 1 = on (day time). |
| `S10` — GPS Lat/Long | `S10:lat=51.507351;lon=-0.127758` | Send GPS latitude and longitude coordinates. Param format: `lat=<latitude>;lon=<longitude>`. Values are in decimal degrees with 6 decimal places precision. |
| `S11` — GPS Altitude | `S11:v=125.50` | Send GPS altitude in meters. Param format: `<sensor>=<value>`. Value is in meters with 2 decimal places precision. |
| `S12` — GPS Speed | `S12:v=15.75;course=245.30;dir=NNW` | Send GPS speed in km/h with optional course/heading in degrees and optional direction. Param format: `v=<speed>;course=<degrees>;dir=<direction>`. Speed is in kilometers per hour with 2 decimal places precision. Course is in degrees (0-360) with 2 decimal places precision, where 0° is North, 90° is East, 180° is South, and 270° is West. Direction is optional cardinal direction string (N, NE, E, SE, S, SW, W, NW, etc.). |
| `S13` — GPS Satellites | `S13:v=8` | Send GPS satellite count. Param format: `<sensor>=<value>`. Value is the number of satellites currently connected to the GPS module. |
| `S14` — GPS Distance | `S14:v=1.23` | Send GPS distance moved. Param format: `<sensor>=<value>`. Value is the total distance travelled according to the GPS module. |


### Wifi Sensor Commands (SFB)
Route: /api/sensor/
Example: Get all sensor values = /api/sensor/

------

## Warning Commands
These commands are used to send warning data from the control panel to link/computer. WarningType corresponds to the enum in WarningManager.h.

| Command | Example | Purpose |
|---|---|---|
| `W0` — Warnings Active | `W0:v=3` | Number of active warnings 3 in this example. Param format: `<value>=<count>`. |
| `W1` — List Warnings | `W1` | Retrieve warning list. Param format: No parameters. |
| `W2` — Warning Status | `W2:0x05=1` | Sends warning status for each warning. Param format: `<WarningType>=<bool>`. |
| `W3` — Clear Warnings | `W3` | Clears all warning data. Param format: No Parameters. |
| `W4` — Warning Set Status | `W4:0x06=1` | Adds or removes a warning to the list of warnings. Param format: `<WarningType>=<bool>`. |


### Wifi Warning Commands (SFB)
Only W1 is supported via wifi.
Route: /api/warning/{command}
Example: Get all active warnings = /api/warning/W1
Returns JSON formatted response with warning status and active warnings if any.

------


## Sound Signal Commands (Fog Horn) (SFB)
These commands are used to activate, query or deactivate signal sounds (Fog Horn Sounds).

| Command | Example | Purpose |
|---|---|---|
| `H0` — Cancel All | `H0` | Cancels any sound signals immediately.. Param format: No Parameters. |
| `H1` — Is Active | `H1` | Retrieves active status of sound signals in form of <SoundType>=<SoundState>. |
| `H2` — Danger | `H2` | Activates SOS horn sound until cancelled ...---... Param format: No Parameters.  |
| `H3` — Fog sound | `H3` | Activates Fog sound (depending on boat type) every 2 minutes until cancelled. Param format: No Parameters. |
| `H4` — Maneuver Starboard | `H4` | Activates maneuver starboard sound. Param format: No Parameters. |
| `H5` — Maneuver Port | `H5` | Activates maneuver port sound. Param format: No Parameters. |
| `H6` — Maneuver Astern | `H6` | Activates maneuver astern sound. Param format: No Parameters. |
| `H7` — Maneuver Danger | `H7` | Activates maneuver danger sound. Param format: No Parameters. |
| `H8` — Overtake Starboard | `H8` | Activates overtake starboard sound. Param format: No Parameters. |
| `H9` — Overtake Port | `H9` | Activates overtake port sound. Param format: No Parameters. |
| `H10` — Overtake Consent | `H10` | Activates overtake consent sound. Param format: No Parameters. |
| `H11` — Overtake Danger | `H11` | Activates overtake danger sound. Param format: No Parameters. |
| `H12` — Test | `H12` | Tests the signal sound. Param format: No Parameters. |


### Wifi Sound Signal Commands (SFB)
Route: /api/sound/{command}
Example: Overtake danger = /api/sound/H11
Returns JSON formatted response with sound status and active sound if any.

------


## Timer / Scheduler Commands (SFB)
These commands manage scheduled events stored in EEPROM. Only available on boards with `SCHEDULER_SUPPORT` enabled (requires ≥ 1KB EEPROM). Up to 20 events can be stored, if EEPROM is < 1KB only 6 events can be stored. Each event has a **trigger** (what fires it), an optional **condition** (a guard that must be true), and an **action** (what happens).

| Command | Example | Purpose |
|---|---|---|
| `T0` — List events | `T0` | Returns the total event count and the enabled state of every slot. Response: `T0:count=3;s=1,0,1,0,...` where `s` is a comma-separated list of enabled states for all 20 slots. |
| `T1` — Get event | `T1:v=2` | Returns full detail of event at index `v` (0..19) using the same format as `T2`. Returns error if slot is empty. |
| `T2` — Set event | `T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0` | Add or update the event at index `i` (0..19). Param format: `i=<index>;e=<enabled>;t=<type,b0,b1,b2,b3>;c=<type,b0,b1,b2,b3>;a=<type,b0,b1,b2,b3>`. Each of `t`, `c`, `a` is a comma-separated list where the first value is the type enum and the remaining four are the payload bytes. Save with `C0` to persist to EEPROM. |
| `T3` — Delete event | `T3:v=2` | Clears the event at index `v` (0..19) and marks the slot as empty. Save with `C0` to persist. |
| `T4` — Enable / disable | `T4:i=2;v=1` | Enable (`v=1`) or disable (`v=0`) the event at index `i` without deleting it. Save with `C0` to persist. |
| `T5` — Clear all | `T5` | Removes all 20 scheduled events. Save with `C0` to persist. |
| `T6` — Trigger now | `T6:v=2` | Immediately executes the action of the event at index `v`, bypassing trigger and condition checks. Useful for testing. Does not affect enabled state. |

**Parameter format for `t`, `c`, `a`:** Each is a 5-value comma-separated string: `<type>,<b0>,<b1>,<b2>,<b3>`. The first value is the type enum; the four byte values are the payload interpreted according to that type. This compact format keeps the total parameter count within the system limit of 5.

---

### Trigger Types (`t` first value)
Relay indices are 0-based (0..`RELAY_COUNT-1`). Multi-byte values use little-endian byte order (low byte first).

| Type | Name | Payload bytes b0..b3 |
|---|---|---|
| `0` | None | Unused — event never fires automatically |
| `1` | TimeOfDay | `b0`=hour (0–23), `b1`=minute (0–59), `b2..b3`=unused |
| `2` | Sunrise | `b0..b1`=int16 offset minutes (positive=after, negative=before), `b2..b3`=unused |
| `3` | Sunset | `b0..b1`=int16 offset minutes (positive=after, negative=before), `b2..b3`=unused |
| `4` | Interval | `b0..b1`=uint16 interval in minutes, `b2..b3`=unused |
| `5` | DayOfWeek | `b0`=bitmask (bit0=Mon, bit1=Tue, bit2=Wed, bit3=Thu, bit4=Fri, bit5=Sat, bit6=Sun), `b1..b3`=unused |
| `6` | Date | `b0`=day (1–31), `b1`=month (1–12), `b2..b3`=unused |

**DayOfWeek bitmask examples:** Mon–Fri = `31`, weekends = `96`, all days = `127`, Monday only = `1`

---

### Condition Types (`c` first value)
Evaluated at trigger time. If the condition is not met the action is skipped. Use `c=0,0,0,0,0` for no condition.

| Type | Name | Payload bytes b0..b3 |
|---|---|---|
| `0` | None | Unused — action always fires when trigger matches |
| `1` | TimeWindow | `b0`=start_hour, `b1`=start_min, `b2`=end_hour, `b3`=end_min |
| `2` | DayOfWeek | `b0`=bitmask (same bit layout as trigger DayOfWeek), `b1..b3`=unused |
| `3` | IsDark | Unused — true when current time is between sunset and sunrise |
| `4` | IsDaylight | Unused — true when current time is between sunrise and sunset |
| `5` | RelayIsOn | `b0`=relay index (0..`RELAY_COUNT-1`), `b1..b3`=unused |
| `6` | RelayIsOff | `b0`=relay index (0..`RELAY_COUNT-1`), `b1..b3`=unused |

---

### Action Types (`a` first value)

| Type | Name | Payload bytes b0..b3 |
|---|---|---|
| `0` | None | Unused — trigger fires but nothing happens |
| `1` | RelayOn | `b0`=relay index, `b1..b3`=unused |
| `2` | RelayOff | `b0`=relay index, `b1..b3`=unused |
| `3` | RelayToggle | `b0`=relay index, `b1..b3`=unused |
| `4` | RelayPulse | `b0`=relay index, `b1..b2`=uint16 duration seconds (little-endian), `b3`=unused |
| `5` | AllRelaysOn | Unused |
| `6` | AllRelaysOff | Unused |

---

### Examples

**"At 18:30 turn Relay 3 off"** (relay index 2, 0-based):
```
T2:i=0;e=1;t=1,18,30,0,0;c=0,0,0,0,0;a=2,2,0,0,0
C0
```

**"10 minutes after sunset turn Relay 2 on"** (relay index 1, offset +10 min):
```
T2:i=1;e=1;t=3,10,0,0,0;c=0,0,0,0,0;a=1,1,0,0,0
C0
```

**"At sunrise, only on weekdays, toggle Relay 1"** (relay index 0, Mon–Fri bitmask=31):
```
T2:i=2;e=1;t=2,0,0,0,0;c=2,31,0,0,0;a=3,0,0,0,0
C0
```

**"Every 30 minutes, pulse Relay 5 for 10 seconds"** (relay index 4, interval=30 min, duration=10 sec):
```
T2:i=3;e=1;t=4,30,0,0,0;c=0,0,0,0,0;a=4,4,10,0,0
C0
```

Common error responses: `Scheduler not supported`, `Index out of range`, `Missing params`, `Invalid trigger type`, `Invalid condition type`, `Invalid action type`, `Invalid enabled value (0 or 1)`, `Slot is empty`.

### Wifi Timer Commands (SFB)
Route: /api/timer/{command}
Example: Get all events = /api/timer/T0
Returns JSON formatted response with event data.

------


## Please Note
- All commands and parameters are case-sensitive.
- Commands must be sent in the exact format as specified, including any required delimiters.
- Commands specific to Boat Control Panel have (BCP) indicated next to them.
- Commands specific to Smart Fuse Box have (SFB) indicated next to them.
- All wifi commands return JSON formatted responses.