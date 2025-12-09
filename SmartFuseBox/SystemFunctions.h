#pragma once
#include <Arduino.h>

class SystemFunctions {
public:
    static uint16_t freeMemory();
    static uint16_t stackAvailable();
    static void initializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection);

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
    static bool parseBooleanValue(const String& value);

    /**
     * @brief Check if a string contains only digits.
     *
     * @param s String to check
     * @return true if the string is non-empty and contains only digits (0-9)
     */
    static bool isAllDigits(const String& s);

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
};