# SD Card Configuration Loader - Manual Implementation Guide

## Overview
This guide lists all manual edits required to complete the SD Card Configuration Loader implementation. The following files have been automatically created:
- ✅ `SmartFuseBox/SdCardConfigLoader.h`
- ✅ `SmartFuseBox/SdCardConfigLoader.cpp`
- ✅ `SD_CONFIG_README.md`

The following files require manual edits due to whitespace matching issues with the editing tool.

---

## Manual Edits Required

### 1. Shared/SystemDefinitions.h
**Location**: After line 74 (after `constexpr char ControlPanelTones[] = "C28";`)

**Add**:
```cpp
constexpr char ConfigReloadFromSd[] = "C29";
constexpr char ConfigExportToSd[] = "C30";
```

---

### 2. Shared/CommandHandlers/ConfigCommandHandler.h
**Location**: Update the class declaration

**Change**:
```cpp
// Forward declarations
class BluetoothController;
class ConfigSyncManager;

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	WifiController* _wifiController;
	ConfigController* _configController;
	ConfigSyncManager* _configSyncManager;

public:
	explicit ConfigCommandHandler(WifiController* wifiController, ConfigController* configController);

	void setConfigSyncManager(ConfigSyncManager* syncManager);
```

**To**:
```cpp
// Forward declarations
class BluetoothController;
class ConfigSyncManager;
class SdCardConfigLoader;  // ADD THIS LINE

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	WifiController* _wifiController;
	ConfigController* _configController;
	ConfigSyncManager* _configSyncManager;
	SdCardConfigLoader* _sdCardConfigLoader;  // ADD THIS LINE

public:
	explicit ConfigCommandHandler(WifiController* wifiController, ConfigController* configController);

	void setConfigSyncManager(ConfigSyncManager* syncManager);
	void setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader);  // ADD THIS LINE
```

---

### 3. Shared/CommandHandlers/ConfigCommandHandler.cpp

#### 3a. Add include at top
**Location**: After `#include "ConfigSyncManager.h"`

**Add**:
```cpp
#include "SdCardConfigLoader.h"
```

#### 3b. Update constructor
**Location**: Lines 10-14

**Change**:
```cpp
ConfigCommandHandler::ConfigCommandHandler(WifiController* wifiController, ConfigController* configController)
	: _wifiController(wifiController),
	  _configController(configController),
	  _configSyncManager(nullptr)
{
}
```

**To**:
```cpp
ConfigCommandHandler::ConfigCommandHandler(WifiController* wifiController, ConfigController* configController)
	: _wifiController(wifiController),
	  _configController(configController),
	  _configSyncManager(nullptr),
	  _sdCardConfigLoader(nullptr)  // ADD THIS LINE
{
}
```

#### 3c. Add setter method
**Location**: After `setConfigSyncManager` method (around line 19)

**Add**:
```cpp
void ConfigCommandHandler::setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader)
{
	_sdCardConfigLoader = sdCardConfigLoader;
}
```

#### 3d. Add C29 and C30 command handlers
**Location**: After the C28 (ControlPanelTones) handler and before the `else` clause (around line 635)

**Add**:
```cpp
    else if (strcmp(command, "C29") == 0)  // ConfigReloadFromSd
    {
        // C29 - Reload config from SD card
        if (_sdCardConfigLoader)
        {
            if (_sdCardConfigLoader->reloadConfigFromSd())
            {
                result = ConfigResult::Success;
            }
            else
            {
                sendAckErr(sender, command, F("Failed to reload from SD"));
                return true;
            }
        }
        else
        {
            sendAckErr(sender, command, F("SD config loader not available"));
            return true;
        }
    }
    else if (strcmp(command, "C30") == 0)  // ConfigExportToSd
    {
        // C30 - Export current config to SD card
        if (_sdCardConfigLoader)
        {
            if (_sdCardConfigLoader->exportConfigToSd())
            {
                result = ConfigResult::Success;
            }
            else
            {
                sendAckErr(sender, command, F("Failed to export to SD"));
                return true;
            }
        }
        else
        {
            sendAckErr(sender, command, F("SD config loader not available"));
            return true;
        }
    }
```

#### 3e. Update supportedCommands method
**Location**: Around line 693

**Change**:
```cpp
const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { 
        ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings, 
        ConfigRename, ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor,
        ConfigBoatType, ConfigSoundRelayId, ConfigSoundStartDelay,
#if defined(ARDUINO_UNO_R4)
        ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID, 
        ConfigWifiPassword, ConfigWifiPort, ConfigWifiState, ConfigWifiApIpAddress,
#endif
        ConfigDefaultRelayState, ConfigLinkRelays,
        ConfigTimeZoneOffset, ConfigMmsi, ConfigCallSign, ConfigHomePort,
        ConfigLedColor, ConfigLedBrightness, ConfigLedAutoSwitch, ConfigLedEnable,
        ControlPanelTones
    };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}
```

**To**:
```cpp
const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { 
        ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings, 
        ConfigRename, ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor,
        ConfigBoatType, ConfigSoundRelayId, ConfigSoundStartDelay,
#if defined(ARDUINO_UNO_R4)
        ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID, 
        ConfigWifiPassword, ConfigWifiPort, ConfigWifiState, ConfigWifiApIpAddress,
#endif
        ConfigDefaultRelayState, ConfigLinkRelays,
        ConfigTimeZoneOffset, ConfigMmsi, ConfigCallSign, ConfigHomePort,
        ConfigLedColor, ConfigLedBrightness, ConfigLedAutoSwitch, ConfigLedEnable,
        ControlPanelTones,
        "C29", "C30"  // ADD THIS LINE
    };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}
```

---

### 4. SmartFuseBox.ino

#### 4a. Add include
**Location**: After `#include "SdCardLogger.h"` (around line 54)

**Add**:
```cpp
#include "SdCardConfigLoader.h"
```

#### 4b. Add SD card config loader instantiation
**Location**: After the SD card logger instantiation (around line 141)

**Add**:
```cpp
// SD card config loader
SdCardConfigLoader sdCardConfigLoader(&commandMgrComputer, &commandMgrLink, &configController, &configSyncManager, SdCardCsPin);
```

#### 4c. Link config loader to config handler
**Location**: In setup() function, after `configHandler.setConfigSyncManager(&configSyncManager);` (around line 177)

**Add**:
```cpp
	configHandler.setSdCardConfigLoader(&sdCardConfigLoader);
```

#### 4d. Load SD config at boot
**Location**: In setup() function, after `sdCardLogger.initialize();` (around line 188)

**Add**:
```cpp
	// Load config from SD card if present (this will disable ConfigSyncManager if SD config found)
	bool sdConfigLoaded = sdCardConfigLoader.loadConfigFromSd();
```

#### 4e. Conditional config sync
**Location**: In setup() function, replace the line `configSyncManager.requestSync();` (around line 202)

**Change**:
```cpp
	configSyncManager.requestSync();
```

**To**:
```cpp
	// Only request config sync if SD config was not loaded
	if (!sdConfigLoaded)
	{
		configSyncManager.requestSync();
	}
```

---

### 5. Commands.md

#### 5a. Add C29 and C30 commands
**Location**: After C28 in the Configuration Commands table

**Add**:
```markdown
| `C29` — Reload Config from SD (SFB) | `C29` | Reload configuration from SD card config.txt file. If successful, applies all settings from SD card, saves to EEPROM, and syncs to control panel via LINK. ConfigSyncManager is disabled when SD config is active. No params. Returns error if SD card not present or config file invalid. |
| `C30` — Export Config to SD (SFB) | `C30` | Export current in-memory configuration to SD card config.txt file. Creates a new file with all current settings in command format, suitable for editing and reloading. Overwrites existing config.txt if present. No params. Returns error if SD card not present or write fails. |
```

#### 5b. Update error responses
**Location**: In the "Common error responses" section (after C28)

**Change**:
```markdown
Common error responses you may see: `Missing param`, `Missing params`, `Missing name`, `Empty name`, `Index out of range`, `Button out of range`, `Slot out of range`, `Relay out of range (or 255 to clear)`, `Invalid value (0 or 1)`, `Invalid mode (0=AP, 1=Client)`, `Only available in Client mode`, `Invalid port`, `Invalid offset (-12 to +14)`, `MMSI must be 9 digits`, `Invalid boat type`, `No available link slots`, `EEPROM commit failed`, `Unknown config command`, `Invalid type (0=day, 1=night)`, `Brightness must be 0-100`, `Invalid value (true/false or 0/1)`, `Missing params (t,r,g,b)`, `Missing params (t,b)`, `Missing params (g,w,s)`.
```

**To**:
```markdown
Common error responses you may see: `Missing param`, `Missing params`, `Missing name`, `Empty name`, `Index out of range`, `Button out of range`, `Slot out of range`, `Relay out of range (or 255 to clear)`, `Invalid value (0 or 1)`, `Invalid mode (0=AP, 1=Client)`, `Only available in Client mode`, `Invalid port`, `Invalid offset (-12 to +14)`, `MMSI must be 9 digits`, `Invalid boat type`, `No available link slots`, `EEPROM commit failed`, `Unknown config command`, `Invalid type (0=day, 1=night)`, `Brightness must be 0-100`, `Invalid value (true/false or 0/1)`, `Missing params (t,r,g,b)`, `Missing params (t,b)`, `Missing params (g,w,s)`, `Failed to reload from SD`, `Failed to export to SD`, `SD config loader not available`.
```

---

## Testing Checklist

After making all manual edits, test the following:

### Compilation
- [ ] Project compiles without errors
- [ ] No warnings related to SD config loader

### Boot Sequence (No SD Card)
- [ ] System boots normally
- [ ] EEPROM config is loaded
- [ ] ConfigSyncManager is enabled
- [ ] Serial shows: "SD card not present or not accessible"

### Boot Sequence (With SD Card + config.txt)
- [ ] System boots and loads SD config
- [ ] EEPROM is updated with SD config
- [ ] ConfigSyncManager is disabled
- [ ] Control panel receives config via LINK
- [ ] Serial shows: "SD config loaded: X commands applied, Y errors"

### C29 Command
- [ ] C29 reloads config from SD card
- [ ] Changes are applied and saved to EEPROM
- [ ] Control panel is synced
- [ ] Returns ACK:C29=ok on success

### C30 Command
- [ ] C30 exports current config to SD card
- [ ] config.txt is created/overwritten
- [ ] File format is correct and parseable
- [ ] Returns ACK:C30=ok on success

### Error Handling
- [ ] Parse errors are logged to Serial
- [ ] Invalid commands don't crash the system
- [ ] Missing SD card returns appropriate error
- [ ] Missing config.txt falls back to EEPROM

---

## Summary

**Files Created**:
- `SmartFuseBox/SdCardConfigLoader.h`
- `SmartFuseBox/SdCardConfigLoader.cpp`
- `SD_CONFIG_README.md`
- `SD_CONFIG_IMPLEMENTATION.md` (this file)

**Files Requiring Manual Edits**:
1. `Shared/SystemDefinitions.h` - Add C29/C30 constants
2. `Shared/CommandHandlers/ConfigCommandHandler.h` - Add SD loader member and setter
3. `Shared/CommandHandlers/ConfigCommandHandler.cpp` - Add C29/C30 handlers
4. `SmartFuseBox.ino` - Integrate SD loader into boot sequence
5. `Commands.md` - Document C29/C30 commands

**Total Manual Edits**: 11 locations across 5 files

For detailed usage instructions and examples, see `SD_CONFIG_README.md`.
