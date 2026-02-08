# SD Card Configuration Loader

## Overview
The SD Card Configuration Loader provides a robust, plug-and-play configuration management system for the SmartFuseBox. It allows users to preconfigure settings on an SD card and automatically apply them at boot, with synchronization to connected control panels.

## Features
- ✅ **Plug-and-Play**: Insert SD card with `config.txt` and settings are automatically applied
- ✅ **EEPROM Persistence**: SD config is saved to EEPROM for retention after card removal
- ✅ **LINK Synchronization**: Automatically syncs configuration to control panel via LINK
- ✅ **ConfigSyncManager Integration**: Automatically disables ConfigSyncManager when SD config is present
- ✅ **Error Logging**: Comprehensive error reporting via Serial
- ✅ **Command Support**: C29 (reload) and C30 (export) commands for runtime management
- ✅ **Validation**: Full command validation before applying changes
- ✅ **Human-Readable Format**: Easy to edit configuration file

## Boot Sequence
1. **Check SD Card**: System checks for SD card presence
2. **Load config.txt**: If found, reads and parses all configuration commands
3. **Validate**: Each command is validated before application
4. **Apply Changes**: Configuration is applied to in-memory config
5. **Save to EEPROM**: If changes were made, saves to EEPROM for persistence
6. **Sync via LINK**: Sends configuration to control panel
7. **Disable ConfigSyncManager**: Prevents periodic sync requests from control panel

If no SD card or config.txt is present, the system boots normally using EEPROM settings and ConfigSyncManager remains enabled.

## Configuration File Format

### File Location
- **Filename**: `config.txt`
- **Location**: Root directory of SD card
- **Encoding**: Plain text (ASCII/UTF-8)

### File Format
The configuration file uses the same command format as the serial protocol:
```
# Comments start with #
# Empty lines are ignored

# Boat identification
C3:Sea Wolf

# Relay names (short|long format)
C4:0=Nav|Navigation
C4:1=Bilge|Bilge Pump
C4:2=Light|Cabin Lights
C4:3=Pump|Water Pump
C4:4=Horn|Sound Horn
C4:5=Fan|Ventilation
C4:6=Spare|Spare 6
C4:7=Spare|Spare 7

# Home button mappings (slot=relay)
C5:0=0
C5:1=1
C5:2=2
C5:3=3

# Button colors (button=color_index)
C6:0=4
C6:1=4
C6:2=4
C6:3=4
C6:4=4
C6:5=4
C6:6=4
C6:7=4

# Vessel type (0=Motor, 1=Sail, 2=Fishing, 3=Yacht)
C7:v=0

# Sound relay (255=unmapped)
C8:v=255

# Sound delay (milliseconds)
C9:v=244

# Bluetooth enabled (0=off, 1=on)
C10:v=0

# WiFi enabled (0=off, 1=on)
C11:v=1

# WiFi mode (0=AP, 1=Client)
C12:v=0

# WiFi SSID (no v= prefix)
C13:SmartFuseBox

# WiFi password (no v= prefix)
C14:sfb-776064

# WiFi port
C15:v=80

# WiFi AP IP address (no v= prefix)
C17:192.168.4.1

# Default relay states (relay=state, 0=off, 1=on)
C18:0=0
C18:1=0
C18:2=0
C18:3=0
C18:4=0
C18:5=0
C18:6=0
C18:7=0

# Linked relays (relay1=relay2, 255=unlink)
C19:255=255
C19:255=255

# Timezone offset (hours from UTC)
C20:v=0

# MMSI (9 digits)
C21:000000000

# Call sign (no v= prefix, can be empty)
C22:

# Home port (no v= prefix, can be empty)
C23:

# LED colors (t=type[0=day,1=night];c=colorset[0=good,1=bad];r=red;g=green;b=blue)
C24:t=0;c=0;r=0;g=80;b=255
C24:t=0;c=1;r=255;g=140;b=0
C24:t=1;c=0;r=100;g=0;b=0
C24:t=1;c=1;r=255;g=50;b=0

# LED brightness (t=type[0=day,1=night];b=brightness[0-100])
C25:t=0;b=80
C25:t=1;b=20

# LED auto switch (0=off, 1=on)
C26:v=1

# LED enable states (g=GPS;w=Warning;s=System)
C27:g=1;w=1;s=1

# Control panel tones (t=type[0=good,1=bad];h=Hz;d=duration;p=preset;r=repeat)
C28:t=0;h=1000;d=100;p=1;r=0
C28:t=1;h=400;d=500;p=5;r=30000
```

### Command Reference
See [Commands.md](Commands.md) for detailed documentation on each command format and parameters.

## Usage

### Initial Setup
1. Create a `config.txt` file with your desired settings
2. Copy the file to the root directory of a FAT32-formatted SD card
3. Insert the SD card into the SmartFuseBox
4. Power on the system
5. Monitor Serial output for confirmation: `SD_CFG_INFO: SD config loaded: X commands applied, Y errors`

### Editing Configuration
1. Power off the system
2. Remove the SD card
3. Edit `config.txt` on your computer
4. Insert the SD card back into the SmartFuseBox
5. Power on (configuration will be reloaded automatically)

**OR** use the C29 command to reload without power cycle:
```
C29
```

### Exporting Current Configuration
To save your current settings to SD card:
```
C30
```
This creates/overwrites `config.txt` with all current settings.

### Removing SD Card Configuration
If you want to go back to control panel-driven configuration:
1. Power off the system
2. Remove the SD card (or delete/rename `config.txt`)
3. Power on the system
4. ConfigSyncManager will be re-enabled automatically
5. Control panel can now manage configuration via C1 sync requests

## Commands

### C29 — Reload Config from SD
**Format**: `C29`  
**Parameters**: None  
**Purpose**: Reload configuration from SD card `config.txt` file

**Behavior**:
- Checks for SD card presence
- Reads and parses `config.txt`
- Applies all valid commands
- Saves changes to EEPROM
- Syncs to control panel via LINK
- Returns `ACK:C29=ok` on success

**Errors**:
- `Failed to reload from SD` - SD card not present, file not found, or parse errors
- `SD config loader not available` - SD card logger not initialized

### C30 — Export Config to SD
**Format**: `C30`  
**Parameters**: None  
**Purpose**: Export current configuration to SD card `config.txt` file

**Behavior**:
- Checks for SD card presence
- Creates/overwrites `config.txt`
- Writes all current settings in command format
- Includes header comment with timestamp
- Returns `ACK:C30=ok` on success

**Errors**:
- `Failed to export to SD` - SD card not present or write failed
- `SD config loader not available` - SD card logger not initialized

## Error Handling

### Parse Errors
If the config loader encounters an error parsing a command:
- Error is logged to Serial: `SD_CFG_ERROR: Command failed: <command> (result=<code>)`
- The problematic line is logged: `SD_CFG_LINE: <line content>`
- Processing continues with remaining commands
- Partial configuration may be applied

### SD Card Issues
If SD card is not present or unreadable:
- System boots using EEPROM configuration
- ConfigSyncManager remains enabled
- Info logged: `SD card not present or not accessible`

### Missing Config File
If SD card is present but `config.txt` is not found:
- System boots using EEPROM configuration
- ConfigSyncManager remains enabled
- Info logged: `Config file not found on SD card`

## Integration with ConfigSyncManager

The SD Card Config Loader intelligently manages ConfigSyncManager:

**When SD config is loaded**:
- `ConfigSyncManager.setEnabled(false)` is called
- Periodic C1 sync requests from control panel are disabled
- SD card becomes the authoritative configuration source
- Control panel receives config via LINK but doesn't initiate syncs

**When SD config is NOT loaded**:
- ConfigSyncManager remains enabled
- Control panel can request config sync via C1
- Bidirectional configuration management is active

This prevents conflicts between SD card configuration and control panel-driven configuration.

## Best Practices

### File Management
- ✅ Keep a backup copy of your `config.txt` on your computer
- ✅ Use meaningful relay names in your configuration
- ✅ Add comments to document your configuration choices
- ✅ Test configuration changes incrementally
- ✅ Use C30 to export working configurations as templates

### Configuration Workflow
1. **Initial Setup**: Use C30 to export default config as starting point
2. **Customize**: Edit exported config on computer
3. **Test**: Load config with SD card, verify behavior
4. **Iterate**: Make adjustments, reload with C29
5. **Deploy**: Copy final config to production SD cards

### Troubleshooting
- Enable Serial monitor to view detailed loading logs
- Check for parse errors in Serial output
- Verify command format matches documentation
- Ensure SD card is FAT32 formatted
- Confirm `config.txt` is in root directory (not in subfolder)
- Use C1 command to verify applied configuration

## Technical Details

### File Reading
- Line-by-line parsing (max 128 characters per line)
- Comments (#) and empty lines are skipped
- Whitespace and newlines are automatically trimmed
- Commands are validated before application

### Memory Management
- Minimal memory footprint during parsing
- No large buffers or caching
- Immediate application of each command
- Single-pass file reading

### SD Card Compatibility
- SdFat library used for robust SD card access
- Supports FAT16/FAT32 file systems
- Compatible with standard SD and SDHC cards
- Chip select pin: D10 (SdCardCsPin)

### Performance
- Boot time impact: ~1-2 seconds for typical config file
- C29 reload: ~1-2 seconds
- C30 export: ~500ms-1 second
- No impact on runtime performance after boot

## Examples

### Minimal Configuration
```
# Minimal config - just boat name and one relay
C3:My Boat
C4:0=Bilge|Bilge Pump
C18:0=0
```

### Advanced Configuration
See the full example at the top of this document.

### Multiple Profiles
Create different config files for different scenarios:
- `config_default.txt` - Standard configuration
- `config_winter.txt` - Winter storage settings
- `config_race.txt` - Racing configuration

Rename the desired profile to `config.txt` before boot.

## Related Documentation
- [Commands.md](Commands.md) - Complete command reference
- [CONFIG_SYNC_README.md](CONFIG_SYNC_README.md) - ConfigSyncManager documentation
- [SdCardLogger.h](SmartFuseBox/SdCardLogger.h) - SD card logging functionality

## Version History
- v1.0.0 - Initial implementation
  - SD card config loading at boot
  - C29 reload command
  - C30 export command
  - ConfigSyncManager integration
