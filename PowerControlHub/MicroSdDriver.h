/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include <Arduino.h>
#include <SdFat.h>
#include <stdint.h>

class WarningManager;

// Configuration constants
constexpr uint8_t SdMaximumOpenFiles = 3;            // Maximum simultaneous open files
constexpr uint16_t SdCardPresenceCheckMs = 5000;       // Card presence check interval (5 seconds)
constexpr uint32_t SdFreeSpaceCacheMs = 3600000;      // Free space cache validity (1 hour)
constexpr uint16_t SdCardSettlingDelayMs = 2000;       // Card settling time after init (2 seconds)

/**
 * @enum SdFileHandle
 * @brief File handle identifiers for managed SD card files
 */
enum class MicroSdFileHandle : uint8_t
{
    Logger = 0,         // Reserved for SdCardLogger
    ConfigLoader = 1,   // Reserved for future SDCardConfigLoader integration
    UserFile1 = 2,      // Available for custom file operations
    Invalid = 0xFF
};

enum class MicroSdInitState : uint8_t
{
    NotInitialized = 0,
    Initializing = 1,
    Settling = 2,
    Initialized = 3,
    Failed = 4
};

/**
 * @struct SdFileInfo
 * @brief Information about a managed file
 */
struct SdFileInfo
{
    FsFile file;
    MicroSdFileHandle handle;
    bool isOpen;
    char fileName[32];

    SdFileInfo()
        : handle(MicroSdFileHandle::Invalid),
          isOpen(false)
    {
        memset(fileName, 0, sizeof(fileName));
    }
};

/**
 * @class SdDiskDriver
 * @brief Centralized SD card resource manager
 * 
 * Architecture:
 * - Singleton pattern for exclusive SD card access control
 * - Manages multiple file handles with resource limits
 * - Provides initialization and presence detection
 * - Thread-safe file operations (for cooperative multitasking)
 * 
 * Design Goals:
 * - Prevent resource conflicts between SdCardLogger and SDCardConfigLoader
 * - Limit number of simultaneously open files (memory constraint)
 * - Provide centralized error handling and card presence detection
 * - Allow temporary exclusive access for critical operations
 * 
 * Integration Points:
 * - SdCardLogger: Uses Logger handle for continuous logging
 * - Future: ConfigLoader handle reserved for SDCardConfigLoader integration
 * - Future components: Can use UserFile1 for additional file operations
 * 
 * Usage Pattern:
 * @code
 * // Initialize driver
 * SdDiskDriver& sdDriver = SdDiskDriver::getInstance();
 * if (!sdDriver.initialize(SdCardCsPin))
 * {
 *     // Handle initialization failure
 * }
 * 
 * // Open a file (from SdCardLogger)
 * FsFile* logFile = sdDriver.openFile(MicroSdFileHandle::Logger, "data.csv", 
 *                                     O_RDWR | O_CREAT | O_APPEND);
 * if (logFile != nullptr)
 * {
 *     logFile->println("Log entry");
 *     sdDriver.closeFile(MicroSdFileHandle::Logger);
 * }
 * 
 * // Request exclusive access for critical operations
 * if (sdDriver.requestExclusiveAccess())
 * {
 *     // Perform critical operations (e.g., firmware update, config reload)
 *     // All open files are closed during exclusive access
 *     sdDriver.releaseExclusiveAccess();
 * }
 * @endcode
 */
// Forward declarations
class MicroSdDriver;

/**
 * @typedef SdCardEventCallback
 * @brief Callback function type for SD card events
 * @param driver Pointer to the MicroSdDriver instance
 * @param isNewCard True if this is a new card (different serial number), false if same card re-initialized
 */
typedef void (*SdCardEventCallback)(bool isNewCard);

class MicroSdDriver
{
private:
	// Singleton instance
	static MicroSdDriver* _instance;

	// Warning manager for status notifications
	WarningManager* _warningManager;

	// Callback for SD card ready events
	SdCardEventCallback _onCardReadyCallback;

	// SD card state
	SdFat _sd;
    uint8_t _misoPin;
    uint8_t _mosiPin;
    uint8_t _sckPin;
	uint8_t _csPin;
	uint32_t _speedMhz;              // Current initialization speed
	MicroSdInitState _initState;
	bool _cardPresent;
	bool _exclusiveAccessActive;

	// File management
	SdFileInfo _files[SdMaximumOpenFiles];
	uint8_t _openFileCount;

	// Statistics
	uint64_t _totalSize;
	uint64_t _freeSpace;
    uint64_t _lastPresenceCheck;
	uint32_t _cardSerialNumber;      // Card serial number for change detection
    uint64_t _lastFreeSpaceUpdate;  // Last time free space was calculated
    uint64_t _lastInitAttemptTime;  // Last initialization attempt time

	// Private constructor (singleton)
	MicroSdDriver();

    // Prevent copying
    MicroSdDriver(const MicroSdDriver&) = delete;
    MicroSdDriver& operator=(const MicroSdDriver&) = delete;

    /**
     * @brief Find file info by handle
     * @param handle File handle to find
     * @return Pointer to SdFileInfo or nullptr if not found
     */
    SdFileInfo* findFileInfo(MicroSdFileHandle handle);

    /**
     * @brief Check if file handle is valid and within range
     * @param handle File handle to validate
     * @return true if handle is valid
     */
    bool isValidHandle(MicroSdFileHandle handle) const;

    /**
     * @brief Update SD card size and free space cache
     * @param forceExpensiveCheck Force recalculation of free space (expensive)
     */
    void updateCardInfo(bool forceExpensiveCheck = false);

    /**
     * @brief Read card serial number from CID register
     * @return Card serial number, or 0 if failed
     */
    uint32_t readCardSerialNumber();

    /**
     * @brief Fast read of total size from FAT32 boot sector
     * @return Total card size in bytes
     */
    uint64_t readTotalSizeFromBootSector();

    /**
     * @brief Check free space and raise/clear low space warning
     */
    void checkFreeSpaceWarning();

public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton SdDiskDriver instance
     */
    static MicroSdDriver& getInstance();

    /**
     * @brief Set warning manager for status notifications
     * @param warningManager Pointer to WarningManager instance
     */
    void setWarningManager(WarningManager* warningManager);

    /**
     * @brief Set callback for SD card ready events
     * 
     * The callback will be invoked when:
     * - Card completes initialization (Settling -> Initialized transition)
     * - A different card is detected during presence check
     * 
     * @param callback Function to call when card becomes ready
     *                 Parameters: (MicroSdDriver* driver, bool isNewCard)
     */
    void setOnCardReadyCallback(SdCardEventCallback callback);

    /**
     * @brief Start non-blocking SD card initialization
     * 
     * Call this once to begin initialization, then call update() in loop()
     * to progress through initialization steps.
     * 
     * @param misoPin SPI MISO pin number (Master In, Slave Out). Pass PinDisabled (0xFF)
     *                if the pin should remain disabled or is not provided by the board.
     * @param mosiPin SPI MOSI pin number (Master Out, Slave In). Pass PinDisabled (0xFF)
     *                if the pin should remain disabled or is not provided by the board.
     * @param sckPin  SPI SCK pin number (Serial Clock). Pass PinDisabled (0xFF)
     *                if the pin should remain disabled or is not provided by the board.
     * @param csPin   Chip select pin for SD card (active low).
     * @param speedMhz SPI speed in MHz (4, 8, 12, 16, 20, or 24)
     *
     * Note: The pin parameter order is explicitly (miso, mosi, sck, cs). This ordering
     * reduces the risk of mixing MOSI/MISO/SCK when calling the function — ensure callers
     * follow the same order. The function stores the provided pin values even on platforms
     * that do not support configuring SPI pins at bus init; in that case the portable
     * SPI.begin() is used and pins remain available for any board-specific configuration
     * helpers if provided by the core.
     */
    void beginInitialize(uint8_t misoPin, uint8_t mosiPin, uint8_t sckPin, uint8_t csPin, uint32_t speedMhz);

    /**
     * @brief Re-initialize SD card (e.g., after card insertion)
     * 
     * Resets state and begins initialization again.
     */
    void reinitialize();

    /**
     * @brief Get current initialization state
     * @return Current SdInitState
     */
    MicroSdInitState getInitState() const
    {
        return _initState;
    }

    /**
     * @brief Check if SD card is initialized and ready
     * @return true if card is ready for operations
     */
    bool isReady() const
    {
        return _initState == MicroSdInitState::Initialized && _cardPresent && !_exclusiveAccessActive;
    }

    /**
     * @brief Check if initialization is complete (success or failure)
     * @return true if initialization is done (not in progress)
     */
    bool isInitComplete() const
    {
        return _initState == MicroSdInitState::Initialized || _initState == MicroSdInitState::Failed;
    }

    /**
     * @brief Check if SD card is physically present
     * @param forceCheck Force immediate hardware check (default: false)
     * @return true if card is present
     */
    bool isCardPresent(bool forceCheck = false);

    /**
     * @brief Open or create a file with specified handle
     * @param handle File handle identifier
     * @param fileName Name of file to open
     * @param oflag Open flags (O_RDWR, O_CREAT, O_APPEND, etc.)
     * @return Pointer to FsFile object or nullptr on failure
     */
    FsFile* openFile(MicroSdFileHandle handle, const char* fileName, oflag_t oflag);

    /**
     * @brief Close a file by handle
     * @param handle File handle to close
     * @return true if file was successfully closed
     */
    bool closeFile(MicroSdFileHandle handle);

    /**
     * @brief Close all open files
     */
    void closeAllFiles();

    /**
     * @brief Check if a specific file is open
     * @param handle File handle to check
     * @return true if file is currently open
     */
    bool isFileOpen(MicroSdFileHandle handle) const;

    /**
     * @brief Get number of currently open files
     * @return Number of open files
     */
    uint8_t getOpenFileCount() const
    {
        return _openFileCount;
    }

    /**
     * @brief Check if a file exists on the SD card
     * @param fileName Name of file to check
     * @return true if file exists
     */
    bool fileExists(const char* fileName);

    /**
     * @brief Delete a file from the SD card
     * @param fileName Name of file to delete
     * @return true if file was successfully deleted
     */
    bool deleteFile(const char* fileName);

    /**
     * @brief Get total SD card size in bytes
     * @return Total card size
     */
    uint64_t getTotalSize() const
    {
        return _totalSize;
    }

    /**
     * @brief Get free space on SD card in bytes
     * @param forceUpdate Force recalculation (expensive operation - ~200-1000ms)
     * @return Free space in bytes
     */
    uint64_t getFreeSpace(bool forceUpdate = false);

    /**
     * @brief Get card serial number from CID register
     * @return 32-bit unique serial number, or 0 if card not initialized
     */
    uint32_t getCardSerialNumber() const
    {
        return _cardSerialNumber;
    }

    /**
     * @brief Request exclusive access to SD card (closes all files)
     * 
     * Use this when you need to perform operations that require
     * exclusive SD card access (e.g., firmware updates, config reloads).
     * 
     * WARNING: This will close ALL open files, including active logs.
     * Caller is responsible for releasing access after operation.
     * 
     * @return true if exclusive access granted
     */
    bool requestExclusiveAccess();

    /**
     * @brief Release exclusive access to SD card
     */
    void releaseExclusiveAccess();

    /**
     * @brief Check if exclusive access is currently active
     * @return true if exclusive access is active
     */
    bool isExclusiveAccessActive() const
    {
        return _exclusiveAccessActive;
    }

    /**
     * @brief Get direct access to SdFat object (advanced use only)
     * 
     * WARNING: Use with caution. Direct access bypasses file handle
     * management and can cause resource conflicts.
     * 
     * @return Pointer to SdFat object or nullptr if not initialized
     */
    SdFat* getSdFat()
    {
        return _initState == MicroSdInitState::Initialized ? &_sd : nullptr;
    }

    /**
     * @brief Update driver state (non-blocking initialization, card presence check)
     * 
     * MUST be called regularly in loop() to progress initialization and
     * maintain card presence detection.
     * 
     * @param now Current time from SystemFunctions::millis64()
     */
    void update(uint64_t now);
};

#endif
