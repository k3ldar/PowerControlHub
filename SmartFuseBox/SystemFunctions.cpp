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
#include "Local.h"
#include "SystemFunctions.h"
#include "SystemDefinitions.h"

#if !defined(WIFI_SUPPORT)
#include <limits.h>
#endif

#if defined(ESP32)
#include "esp_timer.h"
#include <esp_heap_caps.h>
#endif

#if defined(WIFI_SUPPORT) && defined(ARDUINO_UNO_R4)
#include <WiFiS3.h>
#endif

#if defined(WIFI_SUPPORT) && defined(ESP32)
#include <WiFi.h>
#include <esp_mac.h>
#endif

#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
extern "C" char* sbrk(int incr);
#endif

#if defined(ARDUINO_R4_MINIMA)
#include "bsp_api.h"
#endif

uint16_t SystemFunctions::stackAvailable()
{
    extern int __heap_start, * __brkval;
    unsigned int v;
    return (unsigned int)&v - (__brkval == 0 ? (unsigned int)&__heap_start : (unsigned int)__brkval);
}

size_t SystemFunctions::freeMemory()
{
#if defined(ARDUINO_MEGA2560)
    extern int __heap_start, * __brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#elif defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(ESP32)
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
#else
#error "You must define 'ARDUINO_MEGA2560' or 'ARDUINO_UNO_R4' or 'ARDUINO_R4_MINIMA'" or 'ESP32' in your board configuration in Local.h
#endif
}

uint8_t SystemFunctions::GenerateDefaultPassword(char* buffer, size_t bufferSize)
{
    if (!buffer || bufferSize < GeneratedPasswordMinBufferSize)
        return BufferInvalid;

#if defined(WIFI_SUPPORT)
    uint8_t mac[6];
#if defined(ESP32)
    // Read MAC directly from eFuse — safe to call before WiFi.begin() / WiFi.mode()
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
#else
    WiFi.macAddress(mac);
#endif

    snprintf_P(buffer, bufferSize, PSTR("sfb-%02X%02X%02X"), mac[3], mac[4], mac[5]);
#else
    randomSeed(analogRead(A0) + analogRead(A1) + millis());
    uint32_t storedID = static_cast<uint32_t>(random(PasswordRandomMin, LONG_MAX));

    snprintf_P(buffer, bufferSize, PSTR("sfb-%08lX"), (unsigned long)storedID);
#endif

    buffer[bufferSize - 1] = '\0';

    return BufferSuccess;
}

uint32_t SystemFunctions::GetSerialNumber()
{
#if defined(ESP32)
    // eFuse MAC — hardware-burned at manufacture, safe to call before WiFi.begin()
    // Bytes [0..2] are the OUI (same across all ESP32 devices); [2..5] fill the full 32 bits
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    return ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
#elif defined(ARDUINO_UNO_R4)
    // WiFi MAC address — hardware unique per device
    // Bytes [0..2] are the OUI (same across all R4 WiFi devices); [2..5] fill the full 32 bits
    uint8_t mac[6];
    WiFi.macAddress(mac);
    return ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
#elif defined(ARDUINO_R4_MINIMA)
    // RA4M1 128-bit Unique ID via Renesas FSP — factory-programmed, read-only
    // XOR-folded to 32 bits; each word contributes device-specific entropy
    bsp_unique_id_t const* uid = R_BSP_UniqueIdGet();
    return uid->unique_id_words[0] ^ uid->unique_id_words[1] ^
           uid->unique_id_words[2] ^ uid->unique_id_words[3];
#elif defined(ARDUINO_MEGA2560)
    // No unique hardware ID available; generate a random value instead
     randomSeed(analogRead(A0) + analogRead(A1) + millis());
	 return static_cast<uint32_t>(random(PasswordRandomMin, LONG_MAX));
#else
#error "Unrecognized board type — you must define 'ARDUINO_UNO_R4' or 'ARDUINO_R4_MINIMA' or 'ESP32' in your board configuration in Local.h"
#endif
}

void SystemFunctions::initializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection)
{
	serialPort.begin(baudRate);

	if (waitForConnection)
	{
		uint64_t leave = SystemFunctions::millis64() + SerialInitTimeoutMs;

		while (!serialPort && SystemFunctions::millis64() < leave)
			delay(SerialPollingDelayMs);

		if (serialPort)
			delay(SerialStabilizationDelayMs);
	}
}

bool SystemFunctions::parseBooleanValue(const char* value)
{
    if (!value)
        return false;

    return (strcmp(value, "1") == 0 ||
        strcmp(value, "on") == 0 ||
        strcmp(value, "true") == 0);
}

bool SystemFunctions::isAllDigits(const char* s)
{
    if (!s || s[0] == '\0')
        return false;

    for (size_t i = 0; s[i] != '\0'; ++i) 
    {
        if (!isDigit(s[i]))
            return false;
    }

    return true;
}


uint64_t SystemFunctions::elapsedMillis(uint64_t previous)
{
    return SystemFunctions::millis64() - previous;
}

bool SystemFunctions::hasElapsed(uint64_t previous, uint64_t interval)
{
    return (SystemFunctions::millis64() - previous) >= interval;
}

bool SystemFunctions::hasElapsed(uint64_t now, uint64_t previous, uint64_t interval)
{
    return (now - previous) >= interval;
}

void SystemFunctions::resetSerial(Stream& serial)
{
    // Flush outgoing data
    serial.flush();

    // Clear incoming buffer
    while (serial.available() > 0)
    {
        serial.read();
    }
}

bool SystemFunctions::canReboot()
{
#if defined(ESP32)
    return true;
#else
    return false;
#endif
}

void SystemFunctions::reboot()
{
#if defined(ESP32)
    ESP.restart();
#endif
}

bool SystemFunctions::commandMatches(const char* command, const char* expected)
{
    if (strlen(command) != strlen(expected))
        return false;

    return strcmp(command, expected) == 0;
}

bool SystemFunctions::startsWith(const char* str, const char* prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool SystemFunctions::startsWith(const char* str, const __FlashStringHelper* prefix)
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

// Implementation
size_t SystemFunctions::appendString(char* dest, size_t destSize, size_t offset, const char* src)
{
    if (!dest || !src || destSize == 0 || offset >= destSize - 1) return 0;

    size_t available = destSize - offset - 1;
    size_t written = 0;

    while (available > 0 && *src != '\0')
    {
        dest[offset + written++] = *src++;
        available--;
    }

    dest[offset + written] = '\0';
    return written;
}

size_t SystemFunctions::calculateLength(const char* str)
{
    if (!str)
        return 0;
    
    return strlen(str);
}

bool SystemFunctions::substr(char* dest, size_t destSize, const char* src, size_t start, size_t length)
{
    if (!dest || destSize == 0)
        return false;

    size_t srcLen = calculateLength(src);

    if (start >= srcLen)
    {
        dest[0] = '\0';
        return false;
    }

    size_t copyLen = (start + length > srcLen) ? (srcLen - start) : length;

    if (copyLen >= destSize)
    {
        copyLen = destSize - 1;
    }

    for (size_t i = 0; i < copyLen; i++)
    {
        dest[i] = src[start + i];
    }

    dest[copyLen] = '\0';
    return true;
}

bool SystemFunctions::substr(char* dest, size_t destSize, const char* src, size_t start)
{
    size_t srcLen = calculateLength(src);

    if (start >= srcLen)
        return substr(dest, destSize, src, start, 0);

    return substr(dest, destSize, src, start, srcLen - start);
}

int32_t SystemFunctions::indexOf(const char* str, char ch, size_t start = 0)
{
    if (!str || !ch)
        return -1;

	size_t len = calculateLength(str);

    for (size_t i = start; i < len; i++)
    {
        if (str[i] == ch)
            return static_cast<int32_t>(i);
    }

	return -1;
}

void SystemFunctions::wrapTextAtWordBoundary(const char* input, char* output, size_t outputSize, size_t maxLineLength)
{
    if (!input || !output || outputSize == 0)
        return;

    size_t outPos = 0;
    size_t currentLineLength = 0;
    size_t i = 0;

    while (input[i] != '\0' && outPos < outputSize - 1)
    {
        // Handle existing line breaks
        if (input[i] == '\r')
        {
            output[outPos++] = input[i++];
            continue;
        }
        if (input[i] == '\n')
        {
            output[outPos++] = input[i++];
            currentLineLength = 0;
            continue;
        }

        // Skip leading spaces on a new line
        if (currentLineLength == 0 && input[i] == ' ')
        {
            i++;
            continue;
        }

        // Find the next word (up to space, \r, \n, or end)
        size_t wordStart = i;
        size_t wordLength = 0;
        while (input[i] != '\0' && input[i] != ' ' && input[i] != '\r' && input[i] != '\n')
        {
            wordLength++;
            i++;
        }

        // Check if we need to wrap before adding this word
        // Don't wrap if we're at the start of a line or if word itself exceeds max length
        if (currentLineLength > 0 && currentLineLength + wordLength > maxLineLength)
        {
            if (outPos < outputSize - (LineBreakLength + 1))
            {
                output[outPos++] = '\r';
                output[outPos++] = '\n';
                currentLineLength = 0;
            }
        }

        // Copy the word
        for (size_t j = 0; j < wordLength && outPos < outputSize - 1; ++j)
        {
            output[outPos++] = input[wordStart + j];
        }
        currentLineLength += wordLength;

        // Copy trailing spaces (but count them for line length)
        while (input[i] == ' ' && outPos < outputSize - 1)
        {
            output[outPos++] = input[i++];
            currentLineLength++;
        }
    }

    output[outPos] = '\0';
}

void SystemFunctions::sanitizeJsonString(const char* input, char* output, size_t outputSize)
{
    if (!output || outputSize == 0)
        return;

    output[0] = '\0';

    if (!input)
		return;

    size_t outPos = 0;
    const size_t limit = outputSize - 1;

    while (*input != '\0' && outPos < limit)
    {
        uint8_t c = static_cast<uint8_t>(*input++);

        if (c == '\\' || c == '"')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = static_cast<char>(c);
        }
        else if (c == '\b')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = 'b';
        }
        else if (c == '\t')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = 't';
        }
        else if (c == '\n')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = 'n';
        }
        else if (c == '\f')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = 'f';
        }
        else if (c == '\r')
        {
            if (outPos + 2 > limit) break;
            output[outPos++] = '\\';
            output[outPos++] = 'r';
        }
        else if (c < 0x20)
        {
            // Generic control character: \u00XX (6 chars)
            if (outPos + 6 > limit) break;
            static const char hex[] = "0123456789abcdef";
            output[outPos++] = '\\';
            output[outPos++] = 'u';
            output[outPos++] = '0';
            output[outPos++] = '0';
            output[outPos++] = hex[(c >> 4) & 0x0F];
            output[outPos++] = hex[c & 0x0F];
        }
        else
        {
            output[outPos++] = static_cast<char>(c);
        }
    }

    output[outPos] = '\0';
}

bool SystemFunctions::progmemToBuffer(const char* progmemStr, char* buffer, size_t bufferSize)
{
    if (!progmemStr || !buffer || bufferSize == 0)
        return false;

	strncpy_P(buffer, progmemStr, bufferSize);
	buffer[bufferSize - 1] = '\0';
	return true;
}

TimeParts SystemFunctions::msToTimeParts(uint64_t ms)
{
    TimeParts t;
    t.milliseconds = (uint16_t)(ms % MillisecondsPerSecond);

    uint64_t totalSeconds = ms / MillisecondsPerSecond;
    t.seconds = (uint8_t)(totalSeconds % SecondsPerMinute);

    uint64_t totalMinutes = totalSeconds / SecondsPerMinute;
    t.minutes = (uint8_t)(totalMinutes % MinutesPerHour);

    uint64_t totalHours = totalMinutes / MinutesPerHour;
    t.hours = (uint8_t)(totalHours % HoursPerDay);

    t.days = (uint32_t)(totalHours / HoursPerDay);
    return t;
}

uint64_t SystemFunctions::millis64()
{
#if defined(ESP32)
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
#else
    static uint32_t lastMillis32 = 0;
    static uint64_t highMs = 0; 

    uint32_t now = millis();
    
    if (now < lastMillis32)
    {
        // Wrapped around (32-bit overflow)
        highMs += Millis32BitOverflow;
    }

    lastMillis32 = now;
    return highMs + now;
#endif
}

void SystemFunctions::formatTimeParts(char* buffer, size_t bufferSize, const TimeParts& timeparts)
{
    if (!buffer || bufferSize == 0)
        return;

    snprintf(buffer, bufferSize, "%lud %02u:%02u:%02u",
        (unsigned long)timeparts.days,
        (unsigned)timeparts.hours,
        (unsigned)timeparts.minutes,
        (unsigned)timeparts.seconds);
}