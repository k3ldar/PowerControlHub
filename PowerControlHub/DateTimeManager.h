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

#include <Arduino.h>

#include "Local.h"
#include "SystemFunctions.h"
#include "Config.h"

#include "RtcDS1302Driver.h"

constexpr uint8_t DateTimeBufferLength = 20;     // "YYYY-MM-DD HH:MM:SS" + null terminator
constexpr uint8_t DateTimeISOBufferLength = 20;  // "YYYY-MM-DDTHH:MM:SS" + null terminator

/**
 * @class DateTimeManager
 * @brief Static date/time manager for tracking system time with RTC backup support.
 * 
 * This class provides centralized date/time management with optional hardware RTC backup.
 * Time can be updated via GPS, manual commands (F6), or read from RTC on startup.
 * Uses RtcDS1302Driver for hardware abstraction.
 * 
 * Features:
 * - Hardware RTC backup via RtcDS1302Driver
 * - Unix timestamp storage (seconds since epoch)
 * - Millisecond-precision offset tracking
 * - Automatic time progression using SystemFunctions::millis64()
 * - Bidirectional sync: RTC → DateTimeManager on boot, DateTimeManager → RTC on updates
 * - Graceful fallback if RTC is not present
 * 
 * Usage:
 * @code
 * // Initialize RTC and read time on startup
 * DateTimeManager::begin(); // Call this once in setup()
 * 
 * // Set time via GPS, F6 command, or manually (automatically updates RTC)
 * DateTimeManager::setDateTime(1733328600); // Unix timestamp
 * 
 * // Or use ISO 8601 format
 * if (DateTimeManager::setDateTimeISO("2025-12-04T15:30:00"))
 * {
 *     // Successfully parsed, set, and RTC updated
 * }
 * 
 * // Get current time
 * uint64_t currentTime = DateTimeManager::getCurrentTime();
 * 
 * // Check if time has been synchronized
 * if (DateTimeManager::isTimeSet())
 * {
 *     // Time is available
 * }
 * @endcode
 */
class DateTimeManager
{
public:
    /**
     * @brief Initialize with runtime RTC pin configuration from config.
     * If all pins in rtcConfig are valid (not PinDisabled), attempts to
     * initialize DS1302 and read time from hardware. Falls back to default
     * timestamp if RTC is absent or time is invalid.
     * @param rtcConfig RTC pin configuration from Config::rtc
     */
    static void begin(const RtcConfig& rtcConfig);

    /**
     * @brief Set date/time to default value (January 1, 2025 00:00:00).
     * Used during initialization when no external time source is available.
     * Also updates RTC if available.
     */
    static void setDateTime();

    /**
     * @brief Set the current date/time using Unix timestamp.
     * Automatically updates RTC if available.
     * @param unixTimestamp Seconds since Unix epoch (Jan 1, 1970 00:00:00 UTC)
     */
    static void setDateTime(uint64_t unixTimestamp);

    /**
     * @brief Set the current date/time using ISO 8601 format string.
     * Supports format: YYYY-MM-DDTHH:MM:SS
     * @param isoDateTime ISO 8601 formatted date/time string
     * @return true if successfully parsed and set, false otherwise
     */
    static bool setDateTimeISO(const char* isoDateTime);

    /**
     * @brief Get the current Unix timestamp.
     * Calculates elapsed time since last sync using SystemFunctions::millis64().
     * @return Current Unix timestamp (seconds since epoch), 0 if not set
     */
    static uint64_t getCurrentTime();

    /**
     * @brief Check if date/time has been set.
     * @return true if time has been synchronized at least once
     */
    static bool isTimeSet();

    /**
     * @brief Get time elapsed since last sync in seconds.
     * @return Seconds elapsed since setDateTime() was called
     */
    static uint64_t getSecondsSinceSync();

    /**
     * @brief Get the current year.
     * @return Current year (e.g., 2026), or 0 if time not set
     */
    static uint16_t getYear();

    /**
     * @brief Get the current month.
     * @return Current month (1-12), or 0 if time not set
     */
    static uint8_t getMonth();

    /**
     * @brief Get the current day of month.
     * @return Current day (1-31), or 0 if time not set
     */
    static uint8_t getDay();

    /**
     * @brief Get the current hour.
     * @return Current hour (0-23), or 0 if time not set
     */
    static uint8_t getHour();

    /**
     * @brief Get the current minute.
     * @return Current minute (0-59), or 0 if time not set
     */
    static uint8_t getMinute();

    /**
     * @brief Get the current second.
     * @return Current second (0-59), or 0 if time not set
     */
    static uint8_t getSecond();

    /**
     * @brief Format current date/time as ISO 8601 string.
     * Format: YYYY-MM-DDTHH:MM:SS
     * @param buffer Pointer to character buffer to receive formatted string
     * @param bufferLength Size of the buffer (minimum 20 bytes recommended)
     * @return true if time is set and formatted successfully, false if time not synchronized or buffer is null
     */
    static bool formatDateTime(char* buffer, const uint8_t bufferLength);

    /**
     * @brief Format current date/time as human-readable string.
     * Format: YYYY-MM-DD HH:MM:SS
     * @param buffer Pointer to character buffer to receive formatted string
     * @param bufferLength Size of the buffer (minimum 20 bytes recommended)
     * @return true if time is set and formatted successfully, false if time not synchronized or buffer is null
     */
    static bool formatDateTimeReadable(char* buffer, const uint8_t bufferLength);

    /**
     * @brief Reset/clear the stored time.
     * Call this to invalidate current time (e.g., on connection loss).
     */
    static void reset();

    /**
     * @brief Set the timezone offset from UTC.
     * @param offsetHours Hours offset from UTC (-12 to +14)
     */
    static void setTimezoneOffset(int8_t offsetHours);

    /**
     * @brief Get the current timezone offset.
     * @return Hours offset from UTC
     */
    static int8_t getTimezoneOffset();

    /**
     * @brief Check if RTC hardware is available.
     * @return true if RTC hardware is detected and initialized
     */
    static bool isRtcAvailable();

    /**
     * @brief Perform RTC diagnostic check.
     * Delegates to RTC driver for hardware-specific diagnostics.
     * @param buffer Buffer to store diagnostic message
     * @param bufferLength Size of the buffer
     * @return true if all tests pass, false if any test fails
     */
    static bool rtcDiagnostic(char* buffer, const uint8_t bufferLength);

private:
    static RtcDS1302Driver _rtcDriver;
    static uint64_t _syncedTimestamp;
    static uint64_t _syncedMillis;
    static bool _isSet;
    static int8_t _timezoneOffset;

    /**
     * @brief Convert year/month/day/hour/minute/second to Unix timestamp.
     * Simple implementation without leap second handling.
     */
    static uint64_t dateTimeToUnix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

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
    static void unixToDateTime(uint64_t unixTime, uint16_t& year, uint8_t& month, uint8_t& day,
        uint8_t& hour, uint8_t& minute, uint8_t& second);
};

