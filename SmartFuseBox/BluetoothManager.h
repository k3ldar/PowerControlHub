#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>
#include "BluetoothServiceBase.h"
#include "WarningManager.h"

/**
 * @brief Central coordinator for Bluetooth BLE communication.
 * 
 * BluetoothManager is responsible for:
 * - Initializing the BLE stack
 * - Registering services from an array of BluetoothServiceBase instances
 * - Managing connections and advertising
 * - Coordinating periodic service updates
 * 
 * This design mirrors the NextionManager pattern: the manager orchestrates
 * but delegates all domain logic to service instances.
 * 
 * Usage:
 * @code
 * BluetoothSystemService systemService(&systemCommandHandler);
 * BluetoothRelayService relayService(&relayCommandHandler);
 * 
 * BluetoothServiceBase* services[] = { &systemService, &relayService };
 * BluetoothManager btManager(services, 2);
 * 
 * btManager.begin("SmartFuseBox");
 * 
 * void loop() {
 *     btManager.loop();
 * }
 * @endcode
 */
class BluetoothManager
{
public:
    /**
     * @brief Construct a new BluetoothManager with an array of services.
     * 
     * @param services Array of pointers to BluetoothServiceBase instances
     * @param serviceCount Number of services in the array
	 * @param commandMgrComputer Pointer to SerialCommandManager for computer commands
	 * @param warningManager Pointer to WarningManager for warning coordination
     */
    BluetoothManager(SerialCommandManager* commandMgrComputer, WarningManager* warningManager, BluetoothServiceBase** services, uint8_t serviceCount);

    /**
     * @brief Destructor.
     */
    ~BluetoothManager();

    /**
     * @brief Initialize the BLE stack and register all services.
     * 
     * This method:
     * 1. Initializes the BLE stack
     * 2. Creates a BLE server
     * 3. Calls begin() on each registered service
     * 4. Starts advertising
     * 
     * @param deviceName The Bluetooth device name (visible during scanning)
     * @return true if initialization succeeded, false otherwise
     */
    bool begin(const char* deviceName);

    /**
     * @brief Process periodic tasks for all services.
     * 
     * Call this in your main loop(). It will:
     * - Check connection status
     * - Restart advertising if disconnected
     * - Call loop() on each service
     */
    void loop();

    /**
     * @brief Check if a client is currently connected.
     * 
     * @return true if a BLE client is connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Get the number of registered services.
     * 
     * @return uint8_t Number of services
     */
    uint8_t getServiceCount() const;

    /**
     * @brief Get a service by index.
     * 
     * @param index Service index (0-based)
     * @return BluetoothServiceBase* Pointer to service, or nullptr if index out of range
     */
    BluetoothServiceBase* getService(uint8_t index) const;

    /**
     * @brief Start advertising the BLE services.
     * 
     * Called automatically by begin(), but can be called manually if needed
     * (e.g., after stopping advertising).
     */
    void startAdvertising();

    /**
     * @brief Stop advertising the BLE services.
     * 
     * Use this to make the device non-discoverable while maintaining existing connections.
     */
    void stopAdvertising();

    /**
     * @determines whether the device is currently advertising or not.
     */
    bool isAdvertising();

private:
    SerialCommandManager* _commandMgrComputer;
    WarningManager* _warningManager;
    BluetoothServiceBase** _services;
    uint8_t _serviceCount;
    void* _server;  // BLEServer* (void* to avoid exposing BLE library headers)
    bool _isAdvertising;
    bool _deviceConnected;
    unsigned long _lastLoopTime;
    const char* _deviceName;

    /**
     * @brief Internal: Initialize services during begin().
     * 
     * @return true if all services initialized successfully
     */
    bool initializeServices();
};