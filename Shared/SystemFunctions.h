#pragma once
#include <Arduino.h>

class SystemFunctions {
public:
    // Call once at startup (handles any board-specific init)
    static uint16_t freeMemory();
    static uint16_t stackAvailable();
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
     * @brief Check if a pointer is in PROGMEM (flash) or RAM.
     * 
     * @param ptr Pointer to check
     * @return true if pointer is in PROGMEM, false if in RAM
     */
    static bool isProgmem(const char* ptr);

    /**
     * @brief Safely copy a string from PROGMEM or RAM to a buffer.
     * 
     * @param dest Destination buffer
     * @param src Source string (can be PROGMEM or RAM)
     * @param maxLen Maximum bytes to copy (including null terminator)
     * @return Number of characters copied (excluding null terminator)
     */
    static size_t copyString(char* dest, const char* src, size_t maxLen);

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
    static size_t concatStrings(char* dest, size_t destSize, const char* first, Args... args) {
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
    static bool startsWith(const char* str, const char* prefix) {
        return strncmp(str, prefix, strlen(prefix)) == 0;
    }

    /**
    * @brief Check if a string starts with a given prefix stored in PROGMEM.
    *
    * @param str The main string to check (in RAM)
    * @param prefix The prefix to look for (in PROGMEM via F() macro)
    * @return true if str starts with prefix, false otherwise
    */
    static bool startsWith(const char* str, const __FlashStringHelper* prefix) {
        if (!str || !prefix) return false;

        const char* p = reinterpret_cast<const char*>(prefix);
        size_t i = 0;

        // Compare character by character, reading from PROGMEM
        while (true) {
            char prefixChar = pgm_read_byte(p + i);
            if (prefixChar == '\0') {
                return true; // Reached end of prefix, it's a match
            }
            if (str[i] == '\0' || str[i] != prefixChar) {
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

private:
    // Helper: Append a single string to buffer
    static size_t appendString(char* dest, size_t destSize, size_t offset, const char* src);
    
    // Variadic recursion base case
    static size_t concatStringsImpl(char* dest, size_t destSize, size_t offset) {
        (void)dest;
		(void)destSize;
		(void)offset;
        return 0;
    }
    
    // Variadic recursion
    template<typename... Args>
    static size_t concatStringsImpl(char* dest, size_t destSize, size_t offset, const char* first, Args... args) {
        size_t written = appendString(dest, destSize, offset, first);
        return written + concatStringsImpl(dest, destSize, offset + written, args...);
    }
    
    // Calculate total length needed
    
    template<typename... Args>
    static size_t calculateTotalLength(const char* first, Args... args) {
        return calculateLength(first) + calculateTotalLength(args...);
    }
    
    static size_t calculateTotalLength() {
        return 0;
    }
};