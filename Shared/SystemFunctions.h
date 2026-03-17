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
#include <Arduino.h>


struct TimeParts
{
    uint32_t days;
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t milliseconds;
};

class SystemFunctions
{
public:
	/**
	* @brief Estimate free memory (RAM) available.
    * 
	* @return Estimated free memory in bytes
    */
    static size_t freeMemory();

    /**
	* @brief Estimate available stack space.
    * 
	* @return Estimated available stack space in bytes
    */
    static uint16_t stackAvailable();

    /**
     * @brief Generate a default password and write it to the provided buffer.
     *
     * Generates a default password string and stores it in the given buffer.
     * The buffer must be large enough to hold the password and the null terminator.
     *
     * @param buffer Pointer to the character buffer where the password will be written.
     * @param bufferSize Size of the buffer in bytes.
     * @return BufferSuccess (0) if the password was generated and written successfully,
     *         BufferInvalid (nonzero) if the buffer is invalid or too small.
     */
    static uint8_t GenerateDefaultPassword(char* buffer, size_t bufferSize);

    /**
     * @brief Initialize a HardwareSerial port with specified baud rate.
     *
     * Optionally waits for the serial connection to be established (useful for USB serial).
     *
     * @param serialPort Reference to HardwareSerial port (e.g., Serial, Serial1)
     * @param baudRate Baud rate for serial communication
     * @param waitForConnection If true, waits for serial connection to be established
	 */
    static void initializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection);

    /**
     * @brief Parse a string value as a boolean.
     *
     * Accepts multiple formats:
     * - "1" or "0"
     * - "on" or "off" (case-insensitive)
     * - "true" or "false" (case-insensitive)
     *
     * @param value String to parse
     * @return true if the value represents a truthy value, false otherwise
     */
    static bool parseBooleanValue(const char* value);

    /**
     * @brief Check if a string contains only digits.
     *
     * @param s String to check
     * @return true if the string is non-empty and contains only digits (0-9)
     */
    static bool isAllDigits(const char* s);

    /**
     * @brief Calculate elapsed time between two millis() timestamps safely handling wrap-around.
     *
     * This function uses unsigned arithmetic to correctly handle the millis() overflow
     * at ~49.7 days. The subtraction wraps correctly due to unsigned integer behavior.
     *
     * @param now Current timestamp from millis()
     * @param previous Previous timestamp from millis()
     * @return Elapsed milliseconds (handles wrap-around correctly)
     */
    static unsigned long elapsedMillis(unsigned long now, unsigned long previous);

    /**
     * @brief Check if a time interval has elapsed since a previous timestamp.
     *
     * This is a convenience wrapper that handles millis() wrap-around safely.
     *
     * @param now Current timestamp from millis()
     * @param previous Previous timestamp from millis()
     * @param interval Interval to check in milliseconds
     * @return true if interval has elapsed
     */
    static bool hasElapsed(unsigned long now, unsigned long previous, unsigned long interval);

    /**
     * @brief Reset a serial port by flushing outgoing data and clearing incoming buffer.
     *
     * This can help recover from communication issues by ensuring the serial port is in a clean state.
     *
	 * @param serial Reference to the Stream (e.g., HardwareSerial) to reset
    */
    static void resetSerial(Stream& serial)
    {
        // Flush outgoing data
        serial.flush();

        // Clear incoming buffer
        while (serial.available() > 0)
        {
            serial.read();
        }
    }

    /**
     * @brief Concatenate multiple strings into a provided buffer.
     * 
     * Safely concatenates any number of strings (PROGMEM or RAM) into a destination buffer.
     * 
     * @param dest Destination buffer
     * @param destSize Size of destination buffer (including null terminator)
     * @param first First string to concatenate
     * @param args Additional strings to concatenate
     * @return Number of characters written (excluding null terminator), or 0 if buffer too small
     */
    template<typename... Args>
    static size_t concatStrings(char* dest, size_t destSize, const char* first, Args... args)
    {
        if (destSize == 0) return 0;
        
        size_t written = 0;
        dest[0] = '\0';
        
        written += appendString(dest, destSize, written, first);
        written += concatStringsImpl(dest, destSize, written, args...);
        
        return written;
    }

    /**
	* @brief Check if a string starts with a given prefix.
    * 
	* @param str The main string to check
	* @param prefix The prefix to look for
	* @return true if str starts with prefix, false otherwise
    */
    static bool startsWith(const char* str, const char* prefix)
    {
        return strncmp(str, prefix, strlen(prefix)) == 0;
    }

    /**
    * @brief Check if a string starts with a given prefix stored in PROGMEM.
    *
    * @param str The main string to check (in RAM)
    * @param prefix The prefix to look for (in PROGMEM via F() macro)
    * @return true if str starts with prefix, false otherwise
    */
    static bool startsWith(const char* str, const __FlashStringHelper* prefix)
    {
        if (!str || !prefix)
            return false;

        const char* p = reinterpret_cast<const char*>(prefix);
        size_t i = 0;

        // Compare character by character, reading from PROGMEM
        while (true)
        {
            char prefixChar = pgm_read_byte(p + i);

            if (prefixChar == '\0')
            {
                return true; // Reached end of prefix, it's a match
            }

            if (str[i] == '\0' || str[i] != prefixChar)
            {
                return false; // String ended or mismatch
            }
            i++;
        }
    }

    /**
     * @brief Calculate the length of a string (PROGMEM or RAM).
     * 
     * @param str String to measure
     * @return Length of the string (excluding null terminator)
	 */
    static size_t calculateLength(const char* str);

    /**
     * @brief Extract a substring from a source string with specified length.
     *
     * @param dest Destination buffer
     * @param destSize Size of destination buffer (including null terminator)
     * @param src Source string
     * @param start Starting position in source string
     * @param length Number of characters to extract
     * @return true if extraction successful, false if start position is out of bounds
     */
    static bool substr(char* dest, size_t destSize, const char* src, size_t start, size_t length);

    /**
     * @brief Extract a substring from a source string to the end.
     *
     * @param dest Destination buffer
     * @param destSize Size of destination buffer (including null terminator)
     * @param src Source string
     * @param start Starting position in source string
     * @return true if extraction successful, false if start position is out of bounds
     */
    static bool substr(char* dest, size_t destSize, const char* src, size_t start);

    /**
     * @brief Find the first occurrence of a character in a string starting from a position.
     *
     * @param str String to search
     * @param ch Character to find
     * @param start Starting position for search
     * @return Index of first occurrence, or -1 if not found
     */
    static int32_t indexOf(const char* str, char ch, size_t start);

    static void wrapTextAtWordBoundary(const char* input, char* output, size_t outputSize, size_t maxLineLength);

	static bool progmemToBuffer(const char* progmemStr, char* buffer, size_t bufferSize);

	static TimeParts msToTimeParts(uint64_t ms);

    static uint64_t millis64();

    static void formatTimeParts(char* buffer, size_t bufferSize, const TimeParts& timeparts);
private:
    // Helper: Append a single string to buffer
    static size_t appendString(char* dest, size_t destSize, size_t offset, const char* src);
    
    // Variadic recursion base case
    static size_t concatStringsImpl(char* dest, size_t destSize, size_t offset)
    {
        (void)dest;
		(void)destSize;
		(void)offset;
        return 0;
    }
    
    // Variadic recursion
    template<typename... Args>
    static size_t concatStringsImpl(char* dest, size_t destSize, size_t offset, const char* first, Args... args)
    {
        size_t written = appendString(dest, destSize, offset, first);
        return written + concatStringsImpl(dest, destSize, offset + written, args...);
    }
    
    // Calculate total length needed
    
    template<typename... Args>
    static size_t calculateTotalLength(const char* first, Args... args)
    {
        return calculateLength(first) + calculateTotalLength(args...);
    }
    
    static size_t calculateTotalLength()
    {
        return 0;
    }
};