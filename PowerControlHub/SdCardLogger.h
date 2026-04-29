/*
 * PowerControlHub
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
#include "SensorCommandHandler.h"
#include "WarningManager.h"
#include "MicroSdDriver.h"
#include "SystemFunctions.h"

constexpr uint8_t SD_BUFFER_SIZE = 64;              // Number of records to buffer
constexpr uint8_t SD_MAX_WRITES_PER_LOOP = 5;       // Max records to write per update() call
constexpr uint64_t SD_WRITE_INTERVAL_MS = 1000;     // Minimum time between write operations
constexpr uint64_t SD_FILE_CHECK_INTERVAL_MS = 60000; // Check for date change every minute
constexpr uint64_t SD_CARD_PRESENCE_CHECK_MS = 5000; // Check for card presence every 5 seconds

/**
 * @struct SensorSnapshot
 * @brief Snapshot of all sensor values at a point in time
 */
struct SensorSnapshot {
    float temperature;
    uint8_t humidity;
    float bearing;
    float compassTemp;
    uint8_t speed;
    uint16_t waterLevel;
    bool waterPumpActive;
    double gpsLat;
    double gpsLon;
    double altitude;
    double gpsCourse;
    uint32_t gpsSats;
    double gpsDistance;
    uint32_t warnings;
    bool hornActive;

    SensorSnapshot()
        : temperature(NAN), humidity(0), bearing(NAN), compassTemp(NAN), 
          speed(0), waterLevel(0), waterPumpActive(false),
          gpsLat(NAN), gpsLon(NAN), altitude(NAN), gpsCourse(NAN),
          gpsSats(0), gpsDistance(NAN), warnings(0), hornActive(false)
    {
    }
};

/**
 * @class SdCardLogger
 * @brief Non-blocking SD card logger for sensor data
 * 
 * Features:
 * - Subscribes to MessageBus sensor events
 * - Buffers sensor readings in circular buffer
 * - Non-blocking writes (max N records per loop iteration)
 * - Date-based file naming (YYYYMMDD.csv)
 * - Automatic file rotation at midnight
 * - Integrates with WarningManager for error reporting
 * - Configurable via Config struct
 * 
 * Architecture:
 * - Sensor events -> MessageBus -> SdCardLogger buffer -> SD card (chunked writes)
 * 
 * Usage:
 * @code
 * SdCardLogger logger(&messageBus, &warningManager);
 * 
 * void setup()
 * {
 *     Config* config = ConfigManager::getConfigPtr();
 *     if (!logger.initialize(config))
 *     {
 *         // Handle initialization failure
 *     }
 * }
 * 
 * void loop()
 * {
 *     uint64_t now = SystemFunctions::millis64();
 *     logger.update(now);
 * }
 * @endcode
 */
class SdCardLogger
{
private:
	SensorCommandHandler* _sensorHandler;
	WarningManager* _warningManager;

	// SD card (managed by MicroSdDriver)
	FsFile* _currentFile;    // Current log file pointer from MicroSdDriver

	// State
	bool _fileOpen;

    // Circular buffer
    SensorSnapshot _buffer[SD_BUFFER_SIZE];
    uint8_t _bufferHead;        // Next position to write
    uint8_t _bufferTail;        // Next position to read
    uint8_t _bufferCount;       // Number of records in buffer
    char _currentFileName[24];  // "YYYYMMDD.csv\0"
    uint8_t _currentDay;        // Track current day for file rotation
    
    // Timing
    uint64_t _lastWriteTime;
    uint64_t _lastFileCheckTime;
    uint64_t _lastCardPresenceCheck;
    
    // Statistics
    unsigned long _totalRecordsLogged;
    unsigned long _recordsDropped;
    bool _sdCardErrorRaised;
    bool _sdCardMissingRaised;

    uint32_t _initialLogFileSize;

    // Internal methods
    bool openOrCreateFile(uint64_t now);
    void closeCurrentFile();
    bool writeRecordsToCard(uint8_t maxRecords);
    void writeSnapshotToCsv(const SensorSnapshot& snapshot);
    void captureSensorSnapshot();
    void addSnapshotToBuffer(const SensorSnapshot& snapshot);
    bool isBufferFull() const;
    bool isBufferEmpty() const;
    void updateFileName(uint64_t now);
    void checkForDateChange(uint64_t now);
    void checkSdCardStatus();
    
public:
    /**
     * @brief Constructor
     * @param sensorHandler Pointer to SensorCommandHandler for sensor data
     * @param warningManager Pointer to WarningManager for error reporting
     */
    SdCardLogger(SensorCommandHandler* sensorHandler, WarningManager* warningManager);
    
    /**
     * @brief Initialize SD card logger with configuration
     * @param config Configuration structure
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Update logger - processes buffered writes in non-blocking manner
     * @param now Current time from SystemFunctions::millis64()
     */
    void update(uint64_t now);
    
    /**
     * @brief Check if SD card is initialized and working
     * @return true if SD card is ready, false otherwise
     */
    bool isSdCardReady() const;

    /**
     * @brief Get current log file size in bytes
     * @return Log file size in bytes, or 0 if file not open
     */
    uint32_t getCurrentLogFileSize() const;

    /**
     * @brief Get total number of records successfully logged
     * @return Total records logged
     */
    unsigned long getTotalRecordsLogged() const { return _totalRecordsLogged; }
    
    /**
     * @brief Get number of records dropped due to buffer overflow
     * @return Number of dropped records
     */
    unsigned long getRecordsDropped() const { return _recordsDropped; }
    
    /**
     * @brief Get current buffer utilization
     * @return Number of records currently in buffer
     */
    uint8_t getBufferCount() const { return _bufferCount; }
    
    /**
     * @brief Flush all buffered records to SD card (blocking)
     * Use sparingly, typically only during shutdown
     */
    void flush();

    /**
     * @brief Temporarily release SD card access (closes file)
     * Use before operations that need exclusive SD card access (e.g., config reload)
     */
    void releaseSDCard();
};

#endif
