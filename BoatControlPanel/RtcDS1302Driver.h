#pragma once

#include <Arduino.h>
#include "Local.h"

/**
 * @class RtcDS1302Driver
 * @brief Hardware abstraction layer for DS1302 Real-Time Clock.
 * 
 * Provides a clean interface for DS1302 RTC operations including
 * initialization, read/write, and diagnostics. Handles hardware-specific
 * details and provides Unix timestamp interface for easy integration.
 * 
 * Features:
 * - DS1302 hardware initialization and configuration
 * - Read/write Unix timestamps
 * - Hardware diagnostics
 * - Write protection management
 * - Validity checking
 * 
 * Usage:
 * @code
 * RtcDS1302Driver rtc;
 * 
 * rtc.begin();
 * 
 * if (rtc.isAvailable())
 * {
 *     unsigned long timestamp = rtc.readTimestamp();
 *     rtc.writeTimestamp(1735689600);
 * }
 * @endcode
 */
class RtcDS1302Driver
{
public:
    RtcDS1302Driver();
    ~RtcDS1302Driver();

    /**
     * @brief Initialize DS1302 hardware.
     * Sets up communication, disables write protection, starts clock.
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Check if RTC hardware is available and responding.
     * @return true if hardware is present and has valid time
     */
    bool isAvailable() const;

    /**
     * @brief Read current time from RTC as Unix timestamp.
     * @return Unix timestamp (seconds since epoch), or 0 if invalid/unavailable
     */
    unsigned long readTimestamp();

    /**
     * @brief Write Unix timestamp to RTC.
     * @param unixTimestamp Seconds since Unix epoch
     * @return true if write successful, false otherwise
     */
    bool writeTimestamp(unsigned long unixTimestamp);

    /**
     * @brief Perform comprehensive RTC diagnostics.
     * Tests hardware availability, validity, running state, write protection, and time range.
     * @param buffer Buffer to store diagnostic message
     * @param bufferLength Size of the buffer
     * @return true if all diagnostics pass, false if any check fails
     */
    bool runDiagnostics(char* buffer, const uint8_t bufferLength);

private:
    bool _available;
};
