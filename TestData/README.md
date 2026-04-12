# Test Data Files - Quick Reference

## File Locations

All test data files are located in: `C:\GitProjects\SFB\SmartFuseBox\TestData\`

## Files

### 1. config-commands.txt
**Purpose:** Configuration template for SmartFuseBox devices

**Usage:**
```bash
# Serial
SmartFuseSync Config Update -f:"<path>\config-commands.txt" -p:COM3 -w:"SSID" -x:"Pass"

# WiFi
SmartFuseSync WifiConfig Update -f:"<path>\config-commands.txt" -i:"192.168.1.100" -w:"SSID" -x:"Pass"
```

**Contains:**
- System identification (boat name, MMSI, call sign, home port)
- Vessel configuration (type, timezone)
- WiFi settings (with placeholders)
- Bluetooth settings
- MQTT configuration
- Relay naming and configuration
- SPI/SD card settings
- LED configuration
- Control panel tones
- Sensor configuration examples

**Important:**
- WiFi credentials use placeholders: `{{WIFI_SSID}}` and `{{WIFI_PASSWORD}}`
- Must provide `-w` and `-x` parameters when running
- Edit this file to match your device configuration
- Safe to commit to Git (no real credentials)

### 2. test-commands.txt
**Purpose:** Automated test suite for validating device responses via Serial

**Usage:**
```bash
# Serial only
SmartFuseSync Config RunTests -f:"<path>\test-commands.txt" -p:COM3
```

**Contains:**
- System command tests (F0-F13)
- Configuration command tests (C0-C33)
- MQTT command tests (M0-M9)
- Relay command tests (R0-R11)
- Sensor command tests (S0-S6)
- Warning command tests (W1-W4)
- Sound signal tests (H0-H12)
- Timer/scheduler tests (T0-T6)
- Error response tests

**Format:**
```
COMMAND -> EXPECTED_RESPONSE
COMMAND -> RESPONSE1 | RESPONSE2   # Multiple valid responses
COMMAND -> ACK:CMD=*               # Wildcard matching
```

**Important:**
- Responses are pattern-matched
- `*` matches any value
- `|` separates multiple valid responses
- Update expectations if firmware changes

### 3. test-commands-wifi.txt
**Purpose:** Automated test suite for WiFi HTTP API endpoints

**Usage:**
```bash
# WiFi only
SmartFuseSync WifiConfig RunTests -f:"<path>\test-commands-wifi.txt" -i:"192.168.1.100"
```

**Contains:**
- All HTTP API endpoints (GET and POST)
- System commands via /api/system
- Relay commands via /api/relay
- Configuration via /api/config (excluding C11-C17 WiFi settings)
- Sensor queries via /api/sensor
- Warning commands via /api/warning
- Sound signals via /api/sound
- Timer/scheduler via /api/timer
- Error response tests
- Invalid endpoint tests

**Format:**
```
METHOD /endpoint [body] -> JSON_PATTERN
GET /api/relay/R2 -> {"relays":*}
POST /api/config/C3 Test Boat -> {"success":true}
GET /api/invalid -> {"error":*}
```

**Important:**
- Tests actual HTTP endpoints (GET/POST)
- JSON response validation
- `*` wildcards work in JSON patterns
- `|` separates multiple valid JSON responses
- Auto-detects format (WiFi vs Serial)
- **WiFi config commands (C11-C17) are excluded** - these would disconnect the device during testing

## Creating Custom Test Files

### Custom Configuration File

1. Copy `config-commands.txt` to a new file
2. Edit configuration values
3. Keep WiFi placeholders or use actual values
4. Run with your custom file:
   ```bash
   SmartFuseSync config update -f MyConfig.txt -p COM3 -w "SSID" -x "Pass"
   ```

### Custom Test File

1. Create new file with `.txt` extension
2. Use format: `COMMAND -> EXPECTED_RESPONSE`
3. Add comments with `#`
4. Run tests:
   ```bash
   SmartFuseSync config runtests -f MyTests.txt -p COM3
   ```

Example custom test file:
```
# My Custom Tests
# Test basic relay control
R3:0=1 -> ACK:R3=ok
R4:0 -> R4:0=1

# Test MQTT connection
M8 -> M8:v=1
```

## Best Practices

### For Configuration Files
- ✅ Use placeholders for credentials
- ✅ Add comments explaining settings
- ✅ Group related commands
- ✅ End with `C0` to save to EEPROM
- ❌ Never commit real WiFi passwords

### For Test Files
- ✅ Group tests by command category
- ✅ Test both success and error cases
- ✅ Use wildcards for variable data
- ✅ Add descriptive comments
- ✅ Test edge cases (min/max values)

### Security
**Never commit these to Git:**
- `*-local.txt` (local configuration variants)
- `*.bat` files with credentials
- Any file with real passwords

**Add to `.gitignore`:**
```
TestData/*-local.txt
TestData/*.bat
config-local.txt
```

## Updating Test Files

When Commands.md changes:

1. Review new/changed commands
2. Add test cases to `test-commands.txt`
3. Update expected responses if needed
4. Run tests to validate
5. Update `config-commands.txt` if needed

## Quick Commands

```bash
# Update device (Serial with credentials)
SmartFuseSync config update -f TestData/config-commands.txt -p COM3 -w "MyWiFi" -x "MyPass"

# Run all tests (Serial)
SmartFuseSync config runtests -f TestData/test-commands.txt -p COM3

# Update device (WiFi with credentials)
SmartFuseSync wificonfig update -f TestData/config-commands.txt -i 192.168.1.100 -w "MyWiFi" -x "MyPass"

# Run all tests (WiFi)
SmartFuseSync wificonfig runtests -f TestData/test-commands.txt -i 192.168.1.100

# List available ports
SmartFuseSync config listports

# Test connection (Serial)
SmartFuseSync config test -p COM3

# Test connection (WiFi)
SmartFuseSync wificonfig test -i 192.168.1.100
```

## File Format Reference

### Comment Syntax
```
# This is a comment
# Comments can be anywhere
# Blank lines are ignored
```

### Configuration Commands
```
COMMAND:parameters
C11:v=1
M0:v=1
M1:192.168.50.110
```

### Test Commands
```
COMMAND -> EXPECTED
C11:v=1 -> ACK:C11=ok
F2 -> F2:v=*
M3 -> M3:v=*|M3:v=Username
```

### Placeholders
```
C13:{{WIFI_SSID}}
C14:{{WIFI_PASSWORD}}
```

Replaced via CLI:
```bash
-w "ActualSSID" -x "ActualPassword"
```
