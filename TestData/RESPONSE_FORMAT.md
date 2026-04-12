# SmartFuseBox Response Format

## Device Response Pattern

The SmartFuseBox device uses a consistent ACK response format across all commands.

### Response Format

All responses from the device follow this pattern:

```
ACK:COMMAND=STATUS[:additional_data]
```

Where:
- **ACK** = Acknowledgement prefix
- **COMMAND** = The command being acknowledged (e.g., F2, C11, M0)
- **STATUS** = Success/error status (typically `ok` or error message)
- **additional_data** = Optional data returned by the command (prefixed with `:`)

### Examples

#### Query Commands (Return Data)

**Command:** `F2` (Get free memory)  
**Response:** `ACK:F2=ok:v=233008`
- Status: `ok`
- Data: `v=233008` (free memory in bytes)

**Command:** `F5` (WiFi enabled status)  
**Response:** `ACK:F5=ok:v=49;ip=192.168.50.58;ssid=<network-name>;rssi=-45`
- Status: `ok`
- Data: Multiple key-value pairs separated by `;`

**Command:** `M0` (MQTT enabled query)  
**Response:** `ACK:M0=ok:v=1`
- Status: `ok`
- Data: `v=1` (enabled)

#### Set Commands (Configuration Changes)

**Command:** `C11:v=1` (Enable WiFi)  
**Response:** `ACK:C11=ok`
- Status: `ok`
- No additional data

**Command:** `C3:Sea Wolf` (Set boat name)  
**Response:** `ACK:C3=ok:Sea Wolf=`
- Status: `ok`
- Echoes back the value set (with trailing `=`)

**Command:** `R3:0=1` (Turn on relay 0)  
**Response:** `ACK:R3=ok`
- Status: `ok`

#### Error Responses

**Command:** `C20:v=99` (Invalid timezone offset)  
**Response:** `ACK:C20=Invalid offset (-12 to +14)`
- Status: Error message instead of `ok`

**Command:** `R3:99=1` (Invalid relay index)  
**Response:** `ACK:R3=Index out of range`
- Status: Error message

**Command:** `C3` (Missing parameter)  
**Response:** `ACK:C3=Missing name`
- Status: Error message

## Test File Format

Based on this response pattern, test files should use:

### Query Commands (Return Values)
```
COMMAND -> ACK:COMMAND=ok:v=*
```

Examples:
```
F2 -> ACK:F2=ok:v=*
F3 -> ACK:F3=ok:v=*
M0 -> ACK:M0=ok:v=*
```

### Set Commands (No Return Value)
```
COMMAND:params -> ACK:COMMAND=ok
```

Examples:
```
C11:v=1 -> ACK:C11=ok
R3:0=1 -> ACK:R3=ok
M1:192.168.1.100 -> ACK:M1=ok
```

### Set Commands (Echo Value)
```
COMMAND:value -> ACK:COMMAND=ok:*
```

Examples:
```
C3:Test Boat -> ACK:C3=ok:*
C13:TestSSID -> ACK:C13=ok:*
```

### Commands with Reboot Flag
```
COMMAND:params -> ACK:COMMAND=ok;reboot=1
```

Examples:
```
S1:i=0;t=1;o0=0;o1=0 -> ACK:S1=ok;reboot=1
S2:0 -> ACK:S2=ok;reboot=1
```

### Error Responses
```
COMMAND:invalid_params -> ACK:COMMAND=error_message
```

Examples:
```
C20:v=99 -> ACK:C20=Invalid offset (-12 to +14)
R3:99=1 -> ACK:R3=Index out of range
C21:123 -> ACK:C21=MMSI must be 9 digits
```

## Wildcard Usage

Use `*` to match variable content:

- `ACK:F2=ok:v=*` - Matches any numeric value
- `ACK:C3=ok:*` - Matches echoed name with any content
- `ACK:T0=ok:count=*` - Matches count with any value
- `ACK:S0=ok:*` - Matches any sensor configuration data

## Multiple Valid Responses

Use `|` to allow multiple valid responses:

```
C0 -> ACK:C0=ok|SAVED
T1:v=0 -> ACK:T1=ok:*|ACK:T1=Slot is empty
```

## Common Response Patterns

### System Query Commands (F0-F13)
```
F2 -> ACK:F2=ok:v=*        # Free memory
F3 -> ACK:F3=ok:v=*        # CPU usage
F4 -> ACK:F4=ok:v=*        # Bluetooth enabled
F5 -> ACK:F5=ok:v=*        # WiFi enabled (may include ip, ssid, rssi)
F7 -> ACK:F7=ok:v=*        # DateTime
F8 -> ACK:F8=ok:v=*        # SD card present
F9 -> ACK:F9=ok:v=*        # Log file size
F11 -> ACK:F11=ok:v=*      # Uptime
```

### Configuration Set Commands (C0-C33)
```
C3:name -> ACK:C3=ok:*     # Boat name (echoes value)
C7:v=1 -> ACK:C7=ok        # Vessel type
C11:v=1 -> ACK:C11=ok      # WiFi enabled
C20:v=-5 -> ACK:C20=ok     # Timezone
C0 -> ACK:C0=ok            # Save config
```

### MQTT Commands (M0-M9)
```
M0:v=1 -> ACK:M0=ok        # Set MQTT enabled
M0 -> ACK:M0=ok:v=*        # Query MQTT enabled
M1:ip -> ACK:M1=ok         # Set broker
M1 -> ACK:M1=ok:v=*        # Query broker
```

### Relay Commands (R0-R11)
```
R0 -> ACK:R0=ok            # All relays off
R2 -> ACK:R2=ok:*          # Get all states
R3:0=1 -> ACK:R3=ok        # Set relay state
R4:0 -> ACK:R4=ok:0=*      # Get relay state
R5 -> ACK:R5=ok*           # Get all config
```

## Key Differences from Original Expectations

| Original Expected | Actual Device Response | Notes |
|------------------|------------------------|-------|
| `F2:v=*` | `ACK:F2=ok:v=*` | Always includes `ACK:` prefix and `=ok` status |
| `ACK:F2:v=*` | `ACK:F2=ok:v=*` | Status uses `=` not `:` separator |
| `C3=ok` | `ACK:C3=ok:*` | Some commands echo back the value |
| `R2:*` | `ACK:R2=ok:*` | Query commands include ACK and ok status |
| `S0:*` | `ACK:S0=ok:*` | Same pattern for sensor queries |

## Summary

**Golden Rule:** All device responses start with `ACK:COMMAND=ok` for success, with optional additional data after a colon.

**Format Template:**
```
ACK:{COMMAND}={STATUS}[:{DATA}]
```

This consistent format makes it easy to:
1. Identify which command was acknowledged
2. Check if it succeeded (`ok`) or failed (error message)
3. Extract any returned data (after the final `:`)
