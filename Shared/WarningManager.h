#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>
#include <stdint.h>
#include "SharedConstants.h"
#include "WarningType.h"


/**
 * @class WarningManager
 * @brief Simple, centralized warning management with built-in heartbeat monitoring.
 * 
 * This class manages all system warnings including connection heartbeat monitoring.
 * Warnings can be raised/cleared from anywhere in the code. The heartbeat functionality
 * is built-in and automatically manages the ConnectionLost warning.
 * 
 * Features:
 * - Bitmap-based warning tracking using bit flags (supports up to 32 warnings)
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
     * @brief Get the complete bitmask of all active warnings (local + remote).
     * @return uint32_t containing all active warning flags
     */
    uint32_t getActiveWarningsMask() const;

    /**
     * @brief Update remote warning bitmask from connected device.
     * This replaces (not merges) the remote warnings with the new state.
     * @param remoteWarningMask Bitmask of warnings from remote device
     */
    void updateRemoteWarnings(uint32_t remoteWarningMask);

    /**
     * @brief Get only the local warning bitmask.
     * @return uint32_t containing only locally-raised warning flags
     */
    uint32_t getLocalWarningsMask() const;

    /**
     * @brief Get only the remote warning bitmask.
     * @return uint32_t containing only remotely-raised warning flags
     */
    uint32_t getRemoteWarningsMask() const;

private:
    SerialCommandManager* _commandMgr;      // For sending heartbeat commands
    uint32_t _localWarnings;                // Bitmap of LOCAL warnings (raised by this device)
    uint32_t _remoteWarnings;               // Bitmap of REMOTE warnings (from connected device)
    
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
