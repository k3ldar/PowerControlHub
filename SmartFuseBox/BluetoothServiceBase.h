#pragma once

#include <Arduino.h>

/**
 * @brief Abstract base class for Bluetooth BLE services.
 * 
 * This class provides a common interface for all Bluetooth services in the SmartFuseBox.
 * Each service corresponds to a command category from Commands.md (System, Config, Relay, etc.)
 * and encapsulates its own service UUID, characteristics, and command handling logic.
 * 
 * Services delegate to existing command handlers to maintain consistency with serial transport.
 * 
 * Design principles:
 * - Each service owns its UUIDs and characteristics (no centralized config)
 * - Services are self-contained and independently testable
 * - Services reuse existing command handlers (no duplication)
 * - BluetoothManager orchestrates but doesn't contain business logic
 */
class BluetoothServiceBase
{
public:
    virtual ~BluetoothServiceBase() = default;

    /**
     * @brief Initialize the service and register characteristics with BLE stack.
     * 
     * Called by BluetoothManager during setup. Service should create and configure
     * all characteristics, set callbacks, and prepare for operation.
     * 
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief Get the service UUID for this service.
     * 
     * @return const char* Pointer to null-terminated UUID string (e.g., "4fafc201-1fb5-459e-8fcc-c5c9c331914b")
     */
    virtual const char* getServiceUUID() const = 0;

    /**
     * @brief Get a human-readable name for this service.
     * 
     * Used for logging and debugging.
     * 
     * @return const char* Service name (e.g., "System", "Relay", "Config")
     */
    virtual const char* getServiceName() const = 0;

    /**
     * @brief Handle periodic updates (e.g., notifications, heartbeat).
     * 
     * Called periodically by BluetoothManager. Services can use this to:
     * - Send characteristic notifications
     * - Update cached values
     * - Perform health checks
     * 
     * @param currentMillis Current system time in milliseconds
     */
    virtual void loop(unsigned long currentMillis) = 0;

    /**
     * @brief Get the number of characteristics in this service.
     * 
     * Used for diagnostics and validation.
     * 
     * @return uint8_t Number of characteristics registered
     */
    virtual uint8_t getCharacteristicCount() const = 0;

    /**
     * @brief Get the underlying BLEService pointer.
     * 
     * Used by BluetoothManager for advertising.
     * 
     * @return void* Pointer to BLEService (cast to void* to avoid library dependency in base)
     */
    virtual void* getBLEService() = 0;
};