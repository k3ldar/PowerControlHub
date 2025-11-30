#pragma once

#include <Arduino.h>
#include <stdint.h>

/**
 * @enum WarningType
 * @brief Extensible warning types for system monitoring using bit flags.
 *
 * Add new warning types as needed by extending this enum.
 * The manager tracks which warnings are currently active using a bitmap.
 *
 * Bit ranges:
 * - 0-19: System warnings
 * - 20-31: Sensor warnings
 */
enum class WarningType : uint32_t {
    None = 0x00,                                      // No warning

    // System warnings (bits 0-19)
    DefaultConfigurationFuseBox = 1UL << 0,           // 0x00000001 - Using default config
    DefaultConfigurationControlPanel = 1UL << 1,      // 0x00000002 - Using default config
    ConnectionLost = 1UL << 2,                        // 0x00000004 - Link heartbeat lost
    HighCompassTemperature = 1UL << 3,                // 0x00000008 - Compass temperature threshold exceeded
    LowBattery = 1UL << 4,                            // 0x00000010 - Battery voltage low
	BluetoothInitFailed = 1UL << 5,                   // 0x00000020 - Bluetooth initialization failed
	WifiInitFailed = 1UL << 6,                        // 0x00000040 - WiFi initialization failed
	WifiInvalidConfig = 1UL << 7,                     // 0x00000080 - WiFi configuration invalid

    // Sensor warnings (bits 20+)
    SensorFailure = 1UL << 20,                        // 0x00100000 - Sensor communication failure
    TemperatureSensorFailure = 1UL << 21,             // 0x00200000 - Temperature sensor failure
    CompassFailure = 1UL << 22,                       // 0x00400000 - Compass failed to initialize
};

// Warning type display strings - keep in sync with WarningType enum
// Index corresponds to bit position (0-31)
constexpr const char* WARNING_TYPE_STRINGS[32] = {
    "Fuse box Default Configuration",           // Bit 0 - DefaultConfigurationFuseBox
    "Control Panel Default Configuration",      // Bit 1 - DefaultConfigurationControlPanel
    "Connection Lost To Fuse Box",              // Bit 2 - ConnectionLost
    "High Compass Temperature",                 // Bit 3 - HighCompassTemperature
    "Low Battery",                              // Bit 4 - LowBattery
    "",                                         // Bit 5 - Unused
    "",                                         // Bit 6 - Unused
    "",                                         // Bit 7 - Unused
    "",                                         // Bit 8 - Unused
    "",                                         // Bit 9 - Unused
    "",                                         // Bit 10 - Unused
    "",                                         // Bit 11 - Unused
    "",                                         // Bit 12 - Unused
    "",                                         // Bit 13 - Unused
    "",                                         // Bit 14 - Unused
    "",                                         // Bit 15 - Unused
    "",                                         // Bit 16 - Unused
    "",                                         // Bit 17 - Unused
    "",                                         // Bit 18 - Unused
    "",                                         // Bit 19 - Unused
    "Sensor Failure",                           // Bit 20 - SensorFailure
    "Temperature Sensor Failure",               // Bit 21 - TemperatureSensorFailure
    "Compass Failure",                          // Bit 22 - CompassFailure
    "",                                         // Bit 23 - Unused
    "",                                         // Bit 24 - Unused
    "",                                         // Bit 25 - Unused
    "",                                         // Bit 26 - Unused
    "",                                         // Bit 27 - Unused
    "",                                         // Bit 28 - Unused
    "",                                         // Bit 29 - Unused
    "",                                         // Bit 30 - Unused
    ""                                          // Bit 31 - Unused
};

// Helper function to get warning string from bit position
inline const char* getWarningString(uint8_t bitPosition) {
    if (bitPosition < 32) {
        return WARNING_TYPE_STRINGS[bitPosition];  // Return directly (may be "")
    }
    return "";  // Out of range returns empty string
}

// Helper function to get warning string from WarningType
inline const char* getWarningString(WarningType type) {
    if (type == WarningType::None) {
        return "No Warning";
    }

    uint32_t typeValue = static_cast<uint32_t>(type);

    // Find which bit is set (should only be one for valid WarningType)
    for (uint8_t bit = 0; bit < 32; bit++) {
        if (typeValue & (1UL << bit)) {
            return getWarningString(bit);
        }
    }

    return "Unknown Warning";
}

// Bitwise operators for WarningType flags
inline WarningType operator|(WarningType a, WarningType b) {
    return static_cast<WarningType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WarningType operator&(WarningType a, WarningType b) {
    return static_cast<WarningType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline WarningType& operator|=(WarningType& a, WarningType b) {
    return a = a | b;
}
