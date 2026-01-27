#pragma once

#include "GpsSensorHandler.h"

struct SunTimes {
    float sunrise;  // hours since midnight
    float sunset;
    bool isValid;   // false if polar day/night
};

class SunCalculator
{
private:
	GpsSensorHandler* _gpsHandler;
public:
	static SunTimes calculateSunTimes(GpsSensorHandler* gpsHandler);
    static SunTimes calculateSunTimes(float lat, float lon, uint16_t year, uint8_t month, uint8_t day, int8_t tzOffset);
};
