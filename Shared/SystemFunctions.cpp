#include "Local.h"
#include "SystemFunctions.h"
#include "SystemDefinitions.h"

#if defined(ESP32)
#include "esp_timer.h"
#endif

#if defined(WIFI_SUPPORT)
#include <WiFiS3.h>
#endif

#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
extern "C" char* sbrk(int incr);
#endif

uint16_t SystemFunctions::stackAvailable()
{
    extern int __heap_start, * __brkval;
    unsigned int v;
    return (unsigned int)&v - (__brkval == 0 ? (unsigned int)&__heap_start : (unsigned int)__brkval);
}

uint16_t SystemFunctions::freeMemory()
{
#if defined(ARDUINO_MEGA2560)
    extern int __heap_start, * __brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#elif defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
#else
#error "You must define 'ARDUINO_MEGA2560' or 'ARDUINO_UNO_R4' or 'ARDUINO_R4_MINIMA'"
#endif
}

uint8_t SystemFunctions::GenerateDefaultPassword(char* buffer, size_t bufferSize)
{
    if (bufferSize < 15)
        return BufferInvalid;

#if defined(WIFI_SUPPORT)
    uint8_t mac[6];
    WiFi.macAddress(mac);

    snprintf_P(buffer, bufferSize, PSTR("sfb-%02X%02X%02X"), mac[3], mac[4], mac[5]);
#else
    // Use analog noise as seed
    randomSeed(analogRead(A0) + analogRead(A1) + millis());
    uint32_t storedID = random(0x10000000, 0xFFFFFFFF);

    // Format: SFB12A1B2C3
    snprintf_P(buffer, bufferSize, PSTR("sfb-%08X"), static_cast<int>(storedID));

#endif

    buffer[bufferSize - 1] = '\0';

    return BufferSuccess;
}

void SystemFunctions::initializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection)
{
	serialPort.begin(baudRate);

	if (waitForConnection)
	{
		unsigned long leave = millis() + SerialInitTimeoutMs;

		while (!serialPort && millis() < leave)
			delay(10);

		if (serialPort)
			delay(100);
	}
}

bool SystemFunctions::parseBooleanValue(const char* value)
{
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

unsigned long SystemFunctions::elapsedMillis(unsigned long now, unsigned long previous)
{
    return now - previous;
}

bool SystemFunctions::hasElapsed(unsigned long now, unsigned long previous, unsigned long interval)
{
    return (now - previous) >= interval;
}

// Implementation
size_t SystemFunctions::appendString(char* dest, size_t destSize, size_t offset, const char* src) {
    if (!dest || !src || offset >= destSize - 1) return 0;
    
    size_t available = destSize - offset - 1; // Reserve space for null terminator
    size_t written = 0;
    
    while (available > 0 && *src != '\0')
    {
        dest[offset + written++] = *src++;
        available--;
    }
    
    dest[offset + written] = '\0';
    return written;
}

size_t SystemFunctions::calculateLength(const char* str) {
    if (!str)
        return 0;
    
    return strlen(str);
}

bool SystemFunctions::substr(char* dest, size_t destSize, const char* src, size_t start, size_t length)
{
    if (destSize == 0)
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
	return substr(dest, destSize, src, start, calculateLength(src) - start);
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
            if (outPos < outputSize - 3)
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
    t.milliseconds = (uint16_t)(ms % 1000ULL);

    uint64_t totalSeconds = ms / 1000ULL;
    t.seconds = (uint8_t)(totalSeconds % 60ULL);

    uint64_t totalMinutes = totalSeconds / 60ULL;
    t.minutes = (uint8_t)(totalMinutes % 60ULL);

    uint64_t totalHours = totalMinutes / 60ULL;
    t.hours = (uint8_t)(totalHours % 24ULL);

    t.days = (uint32_t)(totalHours / 24ULL);
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
    if (now < lastMillis32) {
        // Wrapped around (32-bit overflow)
        highMs += 0x100000000ULL; // add 2^32
    }
    lastMillis32 = now;
    return highMs + now;
#endif
}

void SystemFunctions::formatTimeParts(char* buffer, uint8_t bufferSize, const TimeParts& timeparts)
{
    snprintf(buffer, bufferSize, "%lud %02u:%02u:%02u",
        (unsigned long)timeparts.days,
        (unsigned)timeparts.hours,
        (unsigned)timeparts.minutes,
        (unsigned)timeparts.seconds);
}