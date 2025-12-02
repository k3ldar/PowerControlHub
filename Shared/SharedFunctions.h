#pragma once
#include <Arduino.h>
#include "Local.h"

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
};