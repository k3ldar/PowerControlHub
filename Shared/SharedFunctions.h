#pragma once
#include <Arduino.h>

class SharedFunctions {
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