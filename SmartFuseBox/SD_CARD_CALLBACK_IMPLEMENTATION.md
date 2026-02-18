# SD Card Callback Notification System

## Summary
Implemented a callback notification system for `MicroSdDriver` to handle dynamic SD card initialization and hot-swap events. This ensures `SDCardConfigLoader` loads configuration files at the correct time, regardless of when the SD card becomes available.

## Problem Solved
Previously, `SmartFuseBox.ino` attempted to load SD card config immediately during `setup()`, but the SD card was only beginning initialization and wouldn't be ready for 2+ seconds (settling period). This caused config loading to fail silently at startup.

Additionally, when users insert a new SD card with updated config, the system needs to detect and load it automatically.

## Implementation Details

### 1. MicroSdDriver Changes

**Header (`MicroSdDriver.h`):**
- Added callback typedef: `typedef void (*SdCardEventCallback)(MicroSdDriver* driver, bool isNewCard);`
- Added private member: `SdCardEventCallback _onCardReadyCallback;`
- Added public method: `void setOnCardReadyCallback(SdCardEventCallback callback);`

**Implementation (`MicroSdDriver.cpp`):**
- Initialize `_onCardReadyCallback` to `nullptr` in constructor
- Implemented `setOnCardReadyCallback()` setter method
- Invoke callback when `Settling` → `Initialized` transition occurs (card ready for first time)
- Invoke callback when different card serial number detected (card swap event)

### 2. SDCardConfigLoader Changes

**Header (`SDCardConfigLoader.h`):**
- Added public method: `void onSdCardReady(bool isNewCard);`

**Implementation (`SDCardConfigLoader.cpp`):**
- Implemented `onSdCardReady()` to:
  - Log appropriate message (initial vs. new card)
  - Call `loadConfigFromSd()`
  - Request config sync from control panel if no SD config found (initial boot only)

### 3. SmartFuseBox.ino Changes

**Setup Modifications:**
- Added forward declaration for `onSdCardReady()` callback
- Registered callback with `microSdDriver.setOnCardReadyCallback(onSdCardReady)`
- Removed immediate call to `sdCardConfigLoader.loadConfigFromSd()`
- Added comments explaining the new callback-based approach

**New Callback Function:**
```cpp
void onSdCardReady(MicroSdDriver* driver, bool isNewCard)
{
    // Forward the callback to the config loader
    sdCardConfigLoader.onSdCardReady(isNewCard);
}
```

## Event Flow

### Initial Startup (No SD Card)
1. `beginInitialize()` called → state = `Initializing`
2. `update()` attempts init → fails → state = `Failed`
3. Auto-retry every 5 seconds
4. When card inserted, init succeeds → `Settling` → `Initialized`
5. **Callback invoked** → config loaded

### Initial Startup (SD Card Present)
1. `beginInitialize()` called → state = `Initializing`
2. `update()` succeeds → state = `Settling`
3. Wait 2000ms (settling delay)
4. State = `Initialized`
5. **Callback invoked** → config loaded
6. If no config file, request sync from control panel

### Card Swap During Runtime
1. Periodic presence check detects different serial number
2. Close all files
3. Update card info
4. **Callback invoked with `isNewCard = true`**
5. Config loaded from new card
6. System automatically applies new configuration

## Benefits

✅ **Correct Timing**: Config loads only when SD card is truly ready
✅ **Hot-Swap Support**: New cards automatically trigger config reload
✅ **Fallback Behavior**: No SD config → request sync from control panel
✅ **Clean Architecture**: Separation of concerns (driver notifies, loader reacts)
✅ **Extensible**: Other components can use the same callback mechanism
✅ **Logging**: Clear messages distinguish initial vs. swap events

## Testing Checklist

- [ ] Build succeeds without errors
- [ ] Boot with no SD card → auto-retry visible
- [ ] Boot with SD card + config.txt → config loads after 2s settling
- [ ] Boot with SD card without config.txt → sync request sent
- [ ] Insert card during runtime → config loads automatically
- [ ] Swap card during runtime → new config loads
- [ ] Remove card during runtime → warning raised, auto-retry starts
- [ ] Re-insert same card → no "new card" message
- [ ] Insert different card → "new card" message logged

## Notes

- The callback is invoked from `MicroSdDriver::update()` which runs in the main loop
- Config loading still uses exclusive access (closes logger temporarily)
- `SdCardLogger` continues to buffer data while SD is unavailable
- Config sync manager is disabled when SD config is loaded (as before)

## Future Enhancements

- Could add callback for card removal events
- Could add callback for low space warnings
- Could batch multiple callbacks if needed (debouncing)
