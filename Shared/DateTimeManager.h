#pragma once

#include <Arduino.h>

/**
 * @class DateTimeManager
 * @brief Static date/time manager for tracking system time.
 * 
 * This class provides centralized date/time management. Initially updated via
 * the F6 system command, it will be replaced with an RTC Clock module later.
 * 
 * Features:
 * - Unix timestamp storage (seconds since epoch)
 * - Millisecond-precision offset tracking
 * - Automatic time progression using millis()
 * - Easy migration path to RTC hardware
 * 
 * Usage:
 * @code
 * // Initialize with current time (F6 command handler)
 * DateTimeManager::setDateTime(1733328600); // Unix timestamp
 * 
 * // Or use ISO 8601 format
 * if (DateTimeManager::setDateTimeISO("2025-12-04T15:30:00")) {
 *     // Successfully parsed and set
 * }
 * 
 * // Get current time
 * unsigned long currentTime = DateTimeManager::getCurrentTime();
 * 
 * // Check if time has been synchronized
 * if (DateTimeManager::isTimeSet()) {
 *     // Time is available
 * }
 * 
 * // Format time as string
 * String timeStr = DateTimeManager::formatDateTime();
 * // Returns: "2025-12-04 15:30:45"
 * @endcode
 */
class DateTimeManager {
public:
    /**
     * @brief Set date/time to default value (January 1, 2025 00:00:00).
     * Used during initialization when no external time source is available.
     */
    static void setDateTime();

    /**
     * @brief Set the current date/time using Unix timestamp.
     * @param unixTimestamp Seconds since Unix epoch (Jan 1, 1970 00:00:00 UTC)
     */
    static void setDateTime(unsigned long unixTimestamp);

    /**
     * @brief Set the current date/time using ISO 8601 format string.
     * Supports format: YYYY-MM-DDTHH:MM:SS
     * @param isoDateTime ISO 8601 formatted date/time string
     * @return true if successfully parsed and set, false otherwise
     */
    static bool setDateTimeISO(const String& isoDateTime);

    /**
     * @brief Get the current Unix timestamp.
     * Calculates elapsed time since last sync using millis().
     * @return Current Unix timestamp (seconds since epoch), 0 if not set
     */
    static unsigned long getCurrentTime();

    /**
     * @brief Check if date/time has been set.
     * @return true if time has been synchronized at least once
     */
    static bool isTimeSet();

    /**
     * @brief Get time elapsed since last sync in seconds.
     * @return Seconds elapsed since setDateTime() was called
     */
    static unsigned long getSecondsSinceSync();

    /**
     * @brief Format current date/time as ISO 8601 string.
     * Format: YYYY-MM-DDTHH:MM:SS
     * @return Formatted date/time string, or "Not Set" if time not synchronized
     */
    static String formatDateTime();

    /**
     * @brief Format current date/time as human-readable string.
     * Format: YYYY-MM-DD HH:MM:SS
     * @return Formatted date/time string, or "Not Set" if time not synchronized
     */
    static String formatDateTimeReadable();

    /**
     * @brief Reset/clear the stored time.
     * Call this to invalidate current time (e.g., on connection loss).
     */
    static void reset();

private:
    static unsigned long _syncedTimestamp;     // Unix timestamp at last sync
    static unsigned long _syncedMillis;        // millis() value at last sync
    static bool _isSet;                        // Has time been synchronized

    /**
     * @brief Convert year/month/day/hour/minute/second to Unix timestamp.
     * Simple implementation without leap second handling.
     */
    static unsigned long dateTimeToUnix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

    /**
     * @brief Convert Unix timestamp to year/month/day/hour/minute/second.
     * @param unixTime Unix timestamp
     * @param year Output year
     * @param month Output month (1-12)
     * @param day Output day (1-31)
     * @param hour Output hour (0-23)
     * @param minute Output minute (0-59)
     * @param second Output second (0-59)
     */
    static void unixToDateTime(unsigned long unixTime, uint16_t& year, uint8_t& month, uint8_t& day,
        uint8_t& hour, uint8_t& minute, uint8_t& second);
};

