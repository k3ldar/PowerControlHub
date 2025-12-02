#include "SharedFunctions.h"
#include "SharedConstants.h"

extern "C" char* sbrk(int incr);

uint16_t SharedFunctions::stackAvailable()
{
    extern int __heap_start, * __brkval;
    unsigned int v;
    return (unsigned int)&v - (__brkval == 0 ? (unsigned int)&__heap_start : (unsigned int)__brkval);
}

uint16_t SharedFunctions::freeMemory()
{
#if defined(ARDUINO_MEGA2560)
    extern int __heap_start, * __brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#elif defined(ARDUINO_UNO_R4)
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
#else
#error "You must define 'ARDUINO_MEGA2560' or 'ARDUINO_UNO_R4'"
#endif
}

void SharedFunctions::initializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection)
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

bool SharedFunctions::parseBooleanValue(const String& value)
{
    return (value == F("1") ||
        value.equalsIgnoreCase(F("on")) ||
        value.equalsIgnoreCase(F("true")));
}

bool SharedFunctions::isAllDigits(const String& s)
{
    if (s.length() == 0)
        return false;

    for (size_t i = 0; i < s.length(); ++i)
    {
        if (!isDigit(s[i]))
            return false;
    }

    return true;
}
