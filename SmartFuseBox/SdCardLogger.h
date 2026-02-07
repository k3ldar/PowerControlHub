#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <stdint.h>
#include "SensorDataRecord.h"
#include "MessageBus.h"
#include "WarningManager.h"

constexpr uint8_t SD_CHIP_SELECT_PIN = 10;
constexpr uint8_t SD_BUFFER_SIZE = 64;              // Number of records to buffer
constexpr uint8_t SD_MAX_WRITES_PER_LOOP = 5;       // Max records to write per update() call
constexpr uint16_t SD_WRITE_INTERVAL_MS = 1000;     // Minimum time between write operations
constexpr uint16_t SD_FILE_CHECK_INTERVAL_MS = 60000; // Check for date change every minute

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
 * void setup() {
 *     Config* config = ConfigManager::getConfigPtr();
 *     if (!logger.initialize(config)) {
 *         // Handle initialization failure
 *     }
 * }
 * 
 * void loop() {
 *     unsigned long now = millis();
 *     logger.update(now);
 * }
 * @endcode
 */
class SdCardLogger {
private:
    MessageBus* _messageBus;
    WarningManager* _warningManager;

    // SD card
    SdFat _sd;              // Main SD card object
    FsFile _currentFile;    // Current log file

    // State
    bool _initialized;
    bool _sdCardPresent;

    // Circular buffer
    SensorDataRecord _buffer[SD_BUFFER_SIZE];
    uint8_t _bufferHead;        // Next position to write
    uint8_t _bufferTail;        // Next position to read
    uint8_t _bufferCount;       // Number of records in buffer
    char _currentFileName[13];  // "YYYYMMDD.csv\0"
    uint8_t _currentDay;        // Track current day for file rotation
    
    // Timing
    unsigned long _lastWriteTime;
    unsigned long _lastFileCheckTime;
    
    // Statistics
    unsigned long _totalRecordsLogged;
    unsigned long _recordsDropped;
    bool _sdCardErrorRaised;
    
    // Internal methods
    bool initializeSdCard();
    bool openOrCreateFile(unsigned long now);
    void closeCurrentFile();
    bool writeRecordsToCard(uint8_t maxRecords);
    void writeRecordToCsv(const SensorDataRecord& record);
    void addRecordToBuffer(const SensorDataRecord& record);
    bool isBufferFull() const;
    bool isBufferEmpty() const;
    void updateFileName(unsigned long now);
    void checkForDateChange(unsigned long now);
    const char* getSensorTypeName(SensorDataType type) const;
    
    // MessageBus callbacks
    void onTemperatureUpdated(float temperature);
    void onHumidityUpdated(uint8_t humidity);
    void onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel);
    void onLightSensorUpdated(bool isDayTime);
    
public:
    /**
     * @brief Constructor
     * @param messageBus Pointer to MessageBus for subscribing to events
     * @param warningManager Pointer to WarningManager for error reporting
     */
    SdCardLogger(MessageBus* messageBus, WarningManager* warningManager);
    
    /**
     * @brief Initialize SD card logger with configuration
     * @param config Configuration structure
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Update logger - processes buffered writes in non-blocking manner
     * @param now Current time from millis()
     */
    void update(unsigned long now);
    
    /**
     * @brief Check if SD card is initialized and working
     * @return true if SD card is ready, false otherwise
     */
    bool isSdCardReady() const { return _initialized && _sdCardPresent; }
    
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
};
