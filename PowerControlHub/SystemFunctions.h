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
#include <cerrno>
#include <climits>
#include <limits>
#include <type_traits>



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
     * @brief Derive a hardware-unique 32-bit serial number from factory-programmed chip identity.
     *
     * Reads a hardware-burned identifier that is fixed for the lifetime of the chip and
     * requires no EEPROM storage. The value is identical across every reboot and survives
     * a factory config reset.
     *
     * - ESP32:           Lower 4 bytes of eFuse MAC (via esp_read_mac)
     * - Arduino R4 WiFi: Lower 4 bytes of WiFi MAC address
     * - Arduino R4 Minima: XOR fold of 128-bit RA4M1 Unique ID registers
     *
     * @return Hardware-unique serial number for this device.
     */
    static uint32_t GetSerialNumber();

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
     * @brief Try to parse an unsigned integer of type T from a string.
     *
     * Accepts decimal or 0x-prefixed hexadecimal formats. Leading '-' is
     * rejected. Uses strtoul/endptr/errno for robust detection and should
     * validate the parsed value fits in the target type T (no overflow).
     *
     * @tparam T unsigned integer type (e.g., uint8_t, uint16_t, uint32_t)
     * @param s Input string to parse
     * @param out Parsed value on success
     * @return true when parsing succeeded and value fits in T, false otherwise
     */
    template<typename T>
    static bool parseUnsigned(const char* s, T& out)
    {
        static_assert(std::is_unsigned<T>::value, "parseUnsigned requires an unsigned type");

        if (!s || *s == '\0' || *s == '-')
            return false;

        char* end = nullptr;
        errno = 0;
        unsigned long v = strtoul(s, &end, 0);

        if (end == s || *end != '\0' || errno == ERANGE)
            return false;

        if (v > static_cast<unsigned long>(std::numeric_limits<T>::max()))
            return false;

        out = static_cast<T>(v);
        return true;
    }
    /**
     * @brief Try to parse a signed integer of type T from a string.
     *
     * Accepts an optional leading '+' or '-' and parses as base-10. Uses
     * strtol/endptr/errno for robust detection and validates the parsed
     * value fits in the target signed type T (no overflow).
     *
     * @tparam T signed integer type (e.g., int8_t, int16_t)
     * @param s Input string to parse
     * @param out Parsed value on success
     * @return true when parsing succeeded and value fits in T, false otherwise
     */
    template<typename T>
    static bool parseSigned(const char* s, T& out)
    {
        static_assert(std::is_signed<T>::value, "parseSigned requires a signed type");

        if (!s || *s == '\0')
            return false;

        char* end = nullptr;
        errno = 0;
        long v = strtol(s, &end, 10);

        if (end == s || *end != '\0' || errno == ERANGE)
            return false;

        if (v < static_cast<long>(std::numeric_limits<T>::min()) ||
            v > static_cast<long>(std::numeric_limits<T>::max()))
            return false;

        out = static_cast<T>(v);
        return true;
    }

    /**
     * @brief Calculate elapsed time since a previous SystemFunctions::millis64() timestamp.
     *
     * This function uses unsigned arithmetic to correctly handle wrap-around.
     * Since millis64() returns a 64-bit value, overflow will not occur within
     * any practical device lifetime.
     *
     * @param previous Previous timestamp from millis64()
     * @return Elapsed milliseconds since 'previous'
     */
    static uint64_t elapsedMillis(uint64_t previous);

    /**
     * @brief Check if a time interval has elapsed since a previous timestamp.
     *
     * This is a convenience wrapper that handles millis() wrap-around safely.
     *
     * @param previous Previous timestamp from SystemFunctions::millis64()
     * @param interval Interval to check in milliseconds
     * @return true if interval has elapsed
     */
    static bool hasElapsed(uint64_t previous, uint64_t interval);

    /**
     * @brief Check if a time interval has elapsed since a previous timestamp.
     *
     * This is a convenience wrapper that handles millis64() wrap-around safely.
     *
     * @param now  Current timestamp from SystemFunctions::millis64()
     * @param previous Previous timestamp from SystemFunctions::millis64()
     * @param interval Interval to check in milliseconds
     * @return true if interval has elapsed
     */
    static bool hasElapsed(uint64_t now, uint64_t previous, uint64_t interval);

	/**
	 * @brief Reset a serial port by flushing outgoing data and clearing incoming buffer.
	 *
	 * This can help recover from communication issues by ensuring the serial port is in a clean state.
	 *
	 * @param serial Reference to the Stream (e.g., HardwareSerial) to reset
	*/
	static void resetSerial(Stream& serial);

	/**
	 * @brief Returns true if this platform supports a software-triggered reboot.
	 *
	 * @return true on ESP32; false on all other platforms (Arduino R4 WiFi, R4 Minima, Mega 2560).
	 */
	static bool canReboot();

	/**
	 * @brief Perform a software reboot on supported platforms.
	 *
	 * Only has effect when canReboot() returns true (ESP32). On other platforms this is a no-op.
	 */
	static void reboot();

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
     * @brief Check if a command string exactly matches an expected command.
     *
     * Validates length first, then uses strcmp for full string equality to
     * prevent prefix ambiguity where shorter command names (e.g. "R1") would
     * otherwise match longer ones (e.g. "R10", "R11") when strncmp with a
     * fixed length is used.
     *
     * @param command The received command string
     * @param expected The expected command constant to compare against
     * @return true if command exactly equals expected
     */
    static bool commandMatches(const char* command, const char* expected);

    /**
	* @brief Check if a string starts with a given prefix.
    * 
	* @param str The main string to check
	* @param prefix The prefix to look for
	* @return true if str starts with prefix, false otherwise
    */
    static bool startsWith(const char* str, const char* prefix);

    /**
    * @brief Check if a string starts with a given prefix stored in PROGMEM.
    *
    * @param str The main string to check (in RAM)
    * @param prefix The prefix to look for (in PROGMEM via F() macro)
    * @return true if str starts with prefix, false otherwise
    */
    static bool startsWith(const char* str, const __FlashStringHelper* prefix);

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

    /**
     * @brief Escape a string for safe embedding as a JSON string value.
     *
     * Applies full JSON string escaping per RFC 8259:
     *   - `\` -> `\\`
     *   - `"` -> `\"`
     *   - `\b` (0x08), `\t` (0x09), `\n` (0x0A), `\f` (0x0C), `\r` (0x0D) -> named escapes
     *   - Any other byte < 0x20 -> `\u00XX`
     * The output is always null-terminated. Characters whose escape sequence
     * would not fit in the remaining buffer are silently dropped.
     *
     * @param input  Source string (RAM)
     * @param output Destination buffer
     * @param outputSize Size of destination buffer including null terminator
     */
    static void sanitizeJsonString(const char* input, char* output, size_t outputSize);

    /**
     * @brief Escape a string for safe embedding in HTML content.
     *
     * Applies HTML entity escaping for the five special characters:
     *   - `&` -> `&amp;`
     *   - `<` -> `&lt;`
     *   - `>` -> `&gt;`
     *   - `"` -> `&quot;`
     *   - `'` -> `&#39;`
     * The output is always null-terminated. Characters whose escape sequence
     * would not fit in the remaining buffer are silently dropped.
     *
     * @param input  Source string (RAM)
     * @param output Destination buffer
     * @param outputSize Size of destination buffer including null terminator
	 */
	static void escapeHtml(const char* input, char* output, size_t outputSize);

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