# SyncFailed Warning Implementation

## Overview
The ConfigSyncManager raises the `SyncFailed` warning when configuration synchronization fails, allowing the system to alert users to potential sync issues.

## Warning Scenarios

### 1. **No Response from Control Panel**
```cpp
completeSyncIfTimeout() // Called after 5 second timeout
└─> Check if ANY config received (_lastConfigReceived == _lastSyncRequest)
    └─> NO response: raiseWarning(SyncFailed)
```
**Cause**: Control Panel didn't respond to C1 (ConfigGetSettings) request  
**Debug Log**: `"Config sync failed - no response"`  
**Effect**: Triggers fast retry (10 seconds)

### 2. **Save to EEPROM Failed**
```cpp
saveConfigIfChanged()
└─> Config changed
    └─> save() returns ConfigResult::Failed
        └─> raiseWarning(SyncFailed)
```
**Cause**: EEPROM write failed  
**Debug Log**: `"Config sync failed to save"`  
**Effect**: Triggers fast retry (10 seconds)

### 3. **Successful Sync (Warning Cleared)**
```cpp
saveConfigIfChanged()
└─> Config changed
    └─> save() returns ConfigResult::Success
        └─> clearWarning(SyncFailed)
            
OR

└─> No config changes
    └─> clearWarning(SyncFailed)
```
**Debug Log**: `"Config synced and saved"` (if changes) or silent (no changes)  
**Effect**: Returns to normal 5 minute interval

## Code Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    SYNC WITH WARNING                         │
└─────────────────────────────────────────────────────────────┘

Timer Expires
     │
     ▼
sendSyncRequest()
     │
     ├─> Send C1 to Control Panel
     ├─> _lastSyncRequest = now
     └─> _lastConfigReceived = now (same time)
     │
     │ (Wait for responses...)
     │
     ▼
notifyConfigReceived() [called by ConfigCommandHandler]
     │
     └─> _lastConfigReceived = now (updated!)
     │
     │ (5 seconds pass with no more config...)
     │
     ▼
completeSyncIfTimeout()
     │
     ├─> Check: _lastConfigReceived == _lastSyncRequest?
     │   │
     │   ├─> YES (nothing received)
     │   │   └─> ⚠️ raiseWarning(SyncFailed)
     │   │       └─> Debug: "no response"
     │   │
     │   └─> NO (config was received)
     │       └─> saveConfigIfChanged()
     │           │
     │           ├─> Config changed?
     │           │   │
     │           │   ├─> YES
     │           │   │   └─> save()
     │           │   │       │
     │           │   │       ├─> Success
     │           │   │       │   └─> ✅ clearWarning(SyncFailed)
     │           │   │       │       └─> Debug: "saved"
     │           │   │       │
     │           │   │       └─> Failed
     │           │   │           └─> ⚠️ raiseWarning(SyncFailed)
     │           │   │               └─> Debug: "failed to save"
     │           │   │
     │           │   └─> NO (no changes)
     │           │       └─> ✅ clearWarning(SyncFailed)
     │           │           └─> Silent (already in sync)
```

## Integration Changes

### ConfigSyncManager.h
```cpp
class ConfigSyncManager
{
private:
    WarningManager* _warningManager; // ADDED
    
public:
    ConfigSyncManager(SerialCommandManager* commandMgrComputer,
                      SerialCommandManager* linkSerial,
                      ConfigController* configController, 
                      WarningManager* warningManager,  // ADDED
                      unsigned long syncInterval = DefaultConfigSyncIntervalMs,
                      unsigned long retryInterval = FastConfigSyncRetryMs);
};
```

### ConfigSyncManager.cpp
```cpp
// Constructor
ConfigSyncManager::ConfigSyncManager(...)
    : _warningManager(warningManager),  // ADDED
      // ... other members

// completeSyncIfTimeout - detect no response
if (_lastConfigReceived == _lastSyncRequest)
{
    _warningManager->raiseWarning(WarningType::SyncFailed);
}

// saveConfigIfChanged - detect save failure
if (result == ConfigResult::Success)
{
    _warningManager->clearWarning(WarningType::SyncFailed);
}
else
{
    _warningManager->raiseWarning(WarningType::SyncFailed);
}

// getCurrentSyncInterval - smart retry based on warning state
if (_warningManager->isWarningActive(WarningType::SyncFailed))
{
    return _retryInterval;  // 10 seconds
}
return _syncInterval;  // 5 minutes
```

### SmartFuseBox.ino
```cpp
// Pass warningManager to ConfigSyncManager
ConfigSyncManager configSyncManager(&commandMgrComputer,
                                   &commandMgrLink, 
                                   &configController, 
                                   &warningManager);  // ADDED
```

## Testing

### Test 1: No Response from Control Panel
1. Disconnect serial link between devices
2. Wait for sync timer or call requestSync()
3. **Expected**: SyncFailed warning raised after 5 second timeout
4. **Debug log**: `"Config sync failed - no response"`
5. **Behavior**: Retries every 10 seconds

### Test 2: EEPROM Save Failure
1. Simulate EEPROM failure (full storage, hardware issue)
2. Modify config on Control Panel
3. Wait for sync
4. **Expected**: SyncFailed warning raised
5. **Debug log**: `"Config sync failed to save"`
6. **Behavior**: Retries every 10 seconds

### Test 3: Successful Sync with Changes
1. Modify config on Control Panel
2. Wait for sync
3. **Expected**: SyncFailed warning cleared (if previously set)
4. **Debug log**: `"Config synced and saved"`
5. **Behavior**: Returns to 5 minute intervals

### Test 4: Successful Sync without Changes
1. Don't modify config
2. Wait for sync
3. **Expected**: SyncFailed warning cleared, no debug message
4. **Debug log**: Silent
5. **Behavior**: Returns to 5 minute intervals

## WarningType Definition
```cpp
// WarningType.h
SyncFailed = 1UL << 9,  // 0x00000200 - Configuration sync issue detected

// Display string
static const char WT_9[] PROGMEM = "Synchronization Failed";
```

## Benefits

1. **User Awareness**: Alerts users when configs are out of sync
2. **Troubleshooting**: Debug logs pinpoint exact failure point
3. **Automatic Recovery**: Warning auto-clears on successful sync
4. **Smart Retry**: Fast retry (10s) when failing, normal (5min) when working
5. **Multiple Failure Detection**: Catches both network and storage issues

## Summary

| Condition | Warning Action | Sync Interval | Debug Message |
|-----------|---------------|---------------|---------------|
| No response | Raise | 10 seconds | "failed - no response" |
| Save failed | Raise | 10 seconds | "failed to save" |
| Save success | Clear | 5 minutes | "synced and saved" |
| No changes | Clear | 5 minutes | Silent |
