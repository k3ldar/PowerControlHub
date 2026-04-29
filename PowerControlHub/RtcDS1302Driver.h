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
     * @brief Initialize DS1302 hardware using runtime pin values from config.
     * @param dataPin  DS1302 DAT pin
     * @param clockPin DS1302 CLK pin
     * @param resetPin DS1302 RST/CE pin
     * @return true if initialization successful, false otherwise
     */
    bool begin(uint8_t dataPin, uint8_t clockPin, uint8_t resetPin);

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
