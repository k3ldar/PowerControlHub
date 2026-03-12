# External Update Pattern

## Overview
This document describes the pattern used for notifying display pages of external state changes from command handlers.

## Architecture

### Base Library (Generic)
**File:** `BaseDisplayPage.h`

The base class provides a generic virtual method:
```cpp
virtual void handleExternalUpdate(uint8_t updateType, const void* data);
```

**Key Design Decisions:**
- Uses `uint8_t` instead of `String` for update types (fast integer comparison, no memory allocation)
- 256 possible values (0x00-0xFF) is more than sufficient for embedded display applications
- Uses `void*` for data (allows any data structure)
- Default implementation does nothing (opt-in pattern)
- No application-specific dependencies

### Application-Specific Implementation
**File:** `HomePage.h`

Define update types using an enum class:
```cpp
enum class HomePageUpdateType : uint8_t
{
    None = 0x00,
    RelayState = 0x01,
    CompassTemperature = 0x02
};
```

Define data structures for each update type:
```cpp
struct RelayStateUpdate {
    uint8_t relayIndex;  // 0-based relay index (0..7)
    bool isOn;           // true = relay on, false = relay off
};
```

Override the handler in your page class:
```cpp
void HomePage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    if (updateType == static_cast<uint8_t>(HomePageUpdateType::RelayState) && data != nullptr)
    {
        const RelayStateUpdate* update = static_cast<const RelayStateUpdate*>(data);
        // Update UI based on relay state
    }
}
```

### Command Handler Usage
**File:** `AckCommandHandler.cpp`

Command handlers notify the current page:
```cpp
RelayStateUpdate update = { relayIndex, isOn };
BaseDisplayPage* currentPage = _nextionControl->getCurrentPage();

if (currentPage)
{
    currentPage->handleExternalUpdate(
        static_cast<uint8_t>(HomePageUpdateType::RelayState), 
        &update
    );
}
```

## Benefits

### Performance
- ? Integer comparison is ~100x faster than string comparison
- ? No dynamic memory allocation
- ? Minimal flash/RAM footprint (1 byte vs 2+ bytes for uint16_t/strings)
- ? Optimized for 8-bit and 32-bit microcontrollers
- ? 256 values is more than sufficient for any embedded display application

### Maintainability
- ? Type-safe with enum class
- ? Self-documenting with named constants
- ? Easy to add new update types
- ? Compiler catches type mismatches

### Flexibility
- ? Base library remains generic
- ? Each page can define its own update types
- ? Pages only handle updates they care about
- ? No coupling between pages and command handlers

## Capacity Planning

With `uint8_t` (256 values):
- Reserve `0x00` for None/Invalid
- **255 usable values** (0x01-0xFF)
- Typical usage: 5-15 update types per page
- Typical application: 4-8 pages
- **Total capacity: 20-120 update types** (well within limits)

### Optional Organization Strategy
You can organize update types by page range:
```cpp
// HomePage: 0x10-0x1F (16 values)
enum class HomePageUpdateType : uint8_t
{
    None = 0x00,
    RelayState = 0x10,
    CompassTemp = 0x11,
    GPSUpdate = 0x12
};

// SettingsPage: 0x20-0x2F (16 values)
enum class SettingsPageUpdateType : uint8_t
{
    ConfigSaved = 0x20,
    CalibrationUpdate = 0x21
};

// DiagnosticsPage: 0x30-0x3F (16 values)
enum class DiagnosticsPageUpdateType : uint8_t
{
    ErrorLog = 0x30,
    SystemStatus = 0x31
};
```

This provides 16 ranges of 16 values each = 256 total values.

## Example: Adding a New Update Type

1. **Define the update type** (HomePage.h):
```cpp
enum class HomePageUpdateType : uint8_t
{
    None = 0x00,
    RelayState = 0x01,
    CompassTemperature = 0x02,
    GPSUpdate = 0x03  // NEW
};
```

2. **Define the data structure** (HomePage.h):
```cpp
struct GPSUpdate
{
    float latitude;
    float longitude;
    uint32_t timestamp;
};
```

3. **Handle it in your page** (HomePage.cpp):
```cpp
void HomePage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    if (updateType == static_cast<uint8_t>(HomePageUpdateType::GPSUpdate) && data != nullptr)
    {
        const GPSUpdate* gps = static_cast<const GPSUpdate*>(data);
        updateGPSDisplay(gps->latitude, gps->longitude);
    }
    // ... handle other types
}
```

4. **Call from command handler**:
```cpp
GPSUpdate update = { 37.5f, -122.3f, millis() };
currentPage->handleExternalUpdate(
    static_cast<uint8_t>(HomePageUpdateType::GPSUpdate), 
    &update
);
```

## Best Practices

1. **Reserve 0x00 for None/Invalid**: Use `None = 0x00` to represent "no update"
2. **Use hex values**: Makes it clear these are numeric identifiers (0x01, 0x02, etc.)
3. **Document data structures**: Always document what fields each structure contains
4. **Check for nullptr**: Always validate `data != nullptr` before casting
5. **Keep updates small**: Large data should be passed by reference/pointer
6. **Synchronous updates**: Current implementation is synchronous; data must remain valid during call
7. **Organize by range**: Consider using ranges (0x10-0x1F, 0x20-0x2F) if you have many pages

## Why uint8_t Instead of uint16_t?

### Advantages of uint8_t:
- ? **256 values is plenty** - typical embedded UI needs 20-120 total update types
- ? **Saves memory** - 1 byte vs 2 bytes (50% reduction)
- ? **Faster** - 8-bit comparison on 8-bit MCUs (AVR) is more efficient
- ? **More idiomatic** - consistent with other embedded types (relay indices, button counts)
- ? **Cleaner hex** - `0x01` vs `0x0001`

### When would you need uint16_t?
- Large-scale SCADA systems (hundreds of update types)
- Dynamic plugin systems (runtime type allocation)
- Multi-tenant systems (isolated type ranges per tenant)

**None of these apply to embedded display systems!**

## Future Considerations

- **Multiple Pages**: Each page type can define its own update types in their enum
- **Async Updates**: If needed, pages should copy data for later processing
- **Type Safety**: Consider using templates if stronger type checking is needed
- **Logging**: Consider adding debug logging for unhandled update types
- **Range Checking**: Pages can ignore update types outside their expected range
