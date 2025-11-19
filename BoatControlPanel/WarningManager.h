#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>
#include <stdint.h>
#include "BoatControlPanelConstants.h"

/**
 * @enum WarningType
 * @brief Extensible warning types for system monitoring using bit flags.
 * 
 * Add new warning types as needed by extending this enum.
 * The manager tracks which warnings are currently active using a bitmap.
 * 
 * Bit ranges:
 * - 0-19: System warnings
 * - 20-63: Sensor warnings
 */
enum class WarningType : uint32_t {
    None = 0x00,                                // No warning
    
    // System warnings (bits 0-19)
    DefaultConfiguration = 1UL << 0,           // 0x01 - Using default config
    ConnectionLost = 1UL << 1,                 // 0x02 - Link heartbeat lost
    HighCompassTemperature = 1UL << 2,         // 0x04 - Compass temperature threshold exceeded
    LowBattery = 1UL << 3,                     // 0x08 - Battery voltage low
    
    // Sensor warnings (bits 20+)
    SensorFailure = 1UL << 20,                 // Sensor communication failure
    TemperatureSensorFailure = 1UL << 21,      // Temperature sensor failure
    CompassFailure = 1UL << 22,                // Compass failed to initialize
};

// Warning type display strings - keep in sync with WarningType enum
// Index corresponds to bit position (0-31)
constexpr const char* WARNING_TYPE_STRINGS[32] = {
    "Using Default Configuration",      // Bit 0 - DefaultConfiguration
    "Connection Lost To Fuse Box",      // Bit 1 - ConnectionLost
    "High Compass Temperature",         // Bit 2 - HighCompassTemperature
    "Low Battery",                      // Bit 3 - LowBattery
    nullptr,                            // Bit 4 - Unused
    nullptr,                            // Bit 5 - Unused
    nullptr,                            // Bit 6 - Unused
    nullptr,                            // Bit 7 - Unused
    nullptr,                            // Bit 8 - Unused
    nullptr,                            // Bit 9 - Unused
    nullptr,                            // Bit 10 - Unused
    nullptr,                            // Bit 11 - Unused
    nullptr,                            // Bit 12 - Unused
    nullptr,                            // Bit 13 - Unused
    nullptr,                            // Bit 14 - Unused
    nullptr,                            // Bit 15 - Unused
    nullptr,                            // Bit 16 - Unused
    nullptr,                            // Bit 17 - Unused
    nullptr,                            // Bit 18 - Unused
    nullptr,                            // Bit 19 - Unused
    "Sensor Failure",                   // Bit 20 - SensorFailure
    "Temperature Sensor Failure",       // Bit 21 - TemperatureSensorFailure
    "Compass Failure",                  // Bit 22 - CompassFailure
    nullptr,                            // Bit 23 - Unused
    nullptr,                            // Bit 24 - Unused
    nullptr,                            // Bit 25 - Unused
    nullptr,                            // Bit 26 - Unused
    nullptr,                            // Bit 27 - Unused
    nullptr,                            // Bit 28 - Unused
    nullptr,                            // Bit 29 - Unused
    nullptr,                            // Bit 30 - Unused
    nullptr                             // Bit 31 - Unused
};

// Helper function to get warning string from bit position
inline const char* getWarningString(uint8_t bitPosition) {
    if (bitPosition < 32 && WARNING_TYPE_STRINGS[bitPosition] != nullptr) {
        return WARNING_TYPE_STRINGS[bitPosition];
    }
    return "Unknown Warning";
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

/**
 * @class WarningManager
 * @brief Simple, centralized warning management with built-in heartbeat monitoring.
 * 
 * This class manages all system warnings including connection heartbeat monitoring.
 * Warnings can be raised/cleared from anywhere in the code. The heartbeat functionality
 * is built-in and automatically manages the ConnectionLost warning.
 * 
 * Features:
 * - Bitmap-based warning tracking using bit flags (supports up to 64 warnings)
 * - Built-in heartbeat monitoring with automatic F0 command transmission
 * - Extensible WarningType enum for adding new warnings
 * - Query methods to check active warnings
 * 
 * Usage:
 * @code
 * WarningManager warningMgr(&commandMgrLink);
 * 
 * // In loop or refresh:
 * warningMgr.update(millis());
 * 
 * // When ACK:F0=ok received:
 * warningMgr.notifyHeartbeatAck();
 * 
 * // Raise warnings from anywhere:
 * warningMgr.raiseWarning(WarningType::HighCompassTemperature);
 * 
 * // Check warnings:
 * if (warningMgr.hasWarnings()) {
 *     // Show warning indicator
 * }
 * @endcode
 */
class WarningManager {
public:
    /**
     * @brief Constructor.
     * @param commandMgr Pointer to SerialCommandManager for heartbeat commands (optional)
     * @param heartbeatInterval How often to send heartbeat in milliseconds
     * @param heartbeatTimeout Timeout before connection considered lost in milliseconds
     */
    explicit WarningManager(SerialCommandManager* commandMgr, unsigned long heartbeatInterval, unsigned long heartbeatTimeout);

    /**
     * @brief Update heartbeat and check for timeouts.
     * Call this periodically (e.g., from refresh()).
     * @param now Current time in milliseconds (from millis())
     */
    void update(unsigned long now);

    /**
     * @brief Notify that a heartbeat acknowledgement was received.
     * Call this when ACK:F0=ok is received.
     */
    void notifyHeartbeatAck();

    /**
     * @brief Raise (activate) a warning.
     * @param type The warning type to activate
     */
    void raiseWarning(WarningType type);

    /**
     * @brief Clear (deactivate) a warning.
     * @param type The warning type to deactivate
     */
    void clearWarning(WarningType type);

    /**
     * @brief Clear all warnings.
     */
    void clearAllWarnings();

    /**
     * @brief Check if any warnings are active.
     * @return true if at least one warning is active
     */
    bool hasWarnings() const;

    /**
     * @brief Check if a specific warning is active.
     * @param type The warning type to check
     * @return true if the warning is active
     */
    bool isWarningActive(WarningType type) const;

    /**
     * @brief Get the complete bitmask of all active warnings.
     * @return uint32_t containing all active warning flags
     */
    uint32_t getActiveWarningsMask() const;

private:
    SerialCommandManager* _commandMgr;      // For sending heartbeat commands
    uint64_t _activeWarnings;               // Bitmap of active warnings (bit per WarningType)
    
    // Heartbeat state
    unsigned long _heartbeatInterval;       // How often to send heartbeat (ms)
    unsigned long _heartbeatTimeout;        // Timeout before connection lost (ms)
    unsigned long _lastHeartbeatSent;       // When last heartbeat was sent
    unsigned long _lastHeartbeatReceived;   // When last ack was received
    bool _heartbeatEnabled;                 // Is heartbeat active

    /**
     * @brief Send a heartbeat command.
     */
    void sendHeartbeat();

    /**
     * @brief Update connection state based on heartbeat.
     * @param now Current time in milliseconds
     */
    void updateConnection(unsigned long now);
};
