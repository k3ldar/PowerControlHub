# Communication Commands
The following sections indicate the types of commands that can be sent to the Boat Control Panel, with examples and purpose.

## System Commands
These are commands used to configure the system settings and can only be sent from a computer, they are not used for internal communication.

| Command | Example | Purpose |
|---|---|---|
| `F0` — Heart beat | `F0` | Send at rated intervals, if no ACK received indicates there is no connection available between control panel and fuse box. No params. |
| `F1` — System Initialized | `F1` | Sent by the system when initialization is complete to signal readiness. No params. Used to notify connected devices or software that the control panel is ready for operation. |
| `F2` — Free Memory | `F2` | When received will return the amount of free memory. |
| `F3` — Cpu Usage | `F3` | When received will return the current CPU usage. No params. |
| `F4` — Bluetooth Enabled | `F4` | When received will return the current enabled state of bluetooth 0 off 1 on. No params. |
| `F5` — Wifi Enabled | `F5` | When received will return the current enabled state of wifi 0 off 1 on. No params. |
| `F6` — Set DateTime | `F6:v=2025-12-04T15:30:00` or `F6:v=1733328600` | Set current date/time. Accepts ISO 8601 datetime string `YYYY-MM-DDTHH:MM:SS` or Unix timestamp (seconds since epoch). Returns current stored time. |
| `F7` — Get DateTime | `F7` | Get current date/time as ISO 8601 datetime string `YYYY-MM-DDTHH:MM:SS`. |


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
| `C1` — Get settings | `C1` | Request full config. Device replies with multiple commands: `C3 <boatName>`, `C4 <idx>:<shortName|longName>` for each relay, `C5 <slot>:<relay>` for each home-slot mapping, then `OK`. No params. |
| `C2` — Reset settings | `C2` | Request full reset of all config settings. No params. |
| `C3` — Rename boat (BCP) | `C3:Sea Wolf` or `C3:name=SeaWolf` | Set the boat name. Accepts a single token or a `<key>:<value>` pair (value is used if present). Empty name → error. Name is truncated to configured max length. |
| `C4` — Rename relay (BCP) | `C4:2=Bilge` or `C4:2=Bilge\|Bilge Pump` | Rename a relay. Param format: `<idx>=<shortName>` or `<idx>=<shortName|longName>`. `idx` must be 0..7 (`RELAY_COUNT`). If no pipe character or long name provided, the short name is used for both. Short name is truncated to 5 chars (used on home page), long name is truncated to 20 chars (used on buttons page). Missing name → error. |
| `C5` — Map home button (BCP) | `C5:1=3` (map) — `C5:1=255` (unmap) | Map a home-page slot to a relay. Param format: `<slot>:<relay>`. `button` must be 0..3 (`HOME_BUTTONS`). `relay` must be 0..7 or `255` to clear/unmap. |
| `C6` — Map home button color (BCP) | `C6:0=4` (map button 1 to Red when activated) — `C6:1=255` (unmap colors) | Map a home-page button to a color when activated (on). Param format: `<slot>:<relay>`. `slot` must be 0..3 (`ConfigManager::HOME_SLOTS`). `relay` must be 0..7 or `255` to clear/unmap. |
| `C7` — Set vessel type | `C7:v=1` | Set the vessel type. Param format: `v=<type>`. Possible values for `<type>` are: 0 (Motor), 1 (Sail), 2 (Fishing), 3 (Yacht). Uses enum values as defined in `Config.h`. Invalid or missing value → error. |
| `C8` — Sound relay button | `C8:v=3` (map) — `C8:v=255` (unmap) | Map the sound system (horn) to a relay. Param format: `<value>:<relay>`. `button` must be 0..7 (`RELAY_COUNT`). `relay` must be 0..7 or `255` to clear/unmap. |
| `C9` — Sound delay Start | `C9:v=0xFF` | Sets the delay before the sound is started in milliseconds, allows other processing to continue so as sounds are not cut off. Invalid or missing value → error. |
| `C10` — Bluetooth Enabled (SFB) | `C10:v=1` | Sets the enabled state of bluetooth, 0 disabled, 1 enabled, if disabling then a restart is required. Invalid or missing value → error. |
| `C11` — Wifi Enabled (SFB) | `C11:v=1` | Sets the enabled state of wifi, 0 disabled, 1 enabled, if disabling then a restart is required. Invalid or missing value → error. |
| `C12` — Wifi Access Mode (SFB) | `C12:v=1` | Sets access mode, 0 is AP and 1 is client |
| `C13` — Wifi SSID (SFB) | `C13:v=RouterName` | Set's the wifi SSID, only available if access mode is client |
| `C14` — Wifi Password (SFB) | `C14:v=Password123` | Set's the wifi password, only available if access mode is client |
| `C15` — Wifi Port (SFB) | `C15:v=80` | Set's the wifi port, this is the port used to listen on, default value is 80 |
| `C16` — Wifi connection state (SFB) | `C16` | Wifi connection state, no params, returns WifiConnectionState value |
| `C17` — Wifi AP Ip address (SFB) | `C17` | Sets the IP address when using access point mode, default is 192.168.4.1 |


Common error responses you may see: `Missing param`, `Missing params`, `Missing name`, `Empty name`, `Index out of range`, `Slot out of range`, `Relay out of range (or 255 to clear)`, `EEPROM commit failed`, `Unknown config command`.


### Wifi Configuration Commands (SFB)
Route: /api/config/{command}
Example: Specify sound relay button = /api/config/F8?v=3 


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


## Please Note
- All commands and parameters are case-sensitive.
- Commands must be sent in the exact format as specified, including any required delimiters.
- Commands specific to Boat Control Panel have (BCP) indicated next to them.
- Commands specific to Smart Fuse Box have (SFB) indicated next to them.
- All wifi commands return JSON formatted responses.