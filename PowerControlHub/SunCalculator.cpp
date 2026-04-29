/*
 * PowerControlHub
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
#include "SunCalculator.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"

SunTimes SunCalculator::calculateSunTimes(GpsSensorHandler* gpsHandler)
{
	Config* config = ConfigManager::getConfigPtr();

    if (!config || !gpsHandler)
    {
        return { -1, -1, false };
	}

    return calculateSunTimes(
        gpsHandler->getLatitude(),
        gpsHandler->getLongitude(),
        DateTimeManager::getYear(),
        DateTimeManager::getMonth(),
        DateTimeManager::getDay(),
        config->system.timezoneOffset
	);
}

SunTimes SunCalculator::calculateSunTimes(float lat, float lon, uint16_t year, uint8_t month, uint8_t day, int8_t tzOffset)
{
    // Calculate day of year
    int32_t N = day + 31 * (month - 1);

    if (month > 2)
        N -= (4 * month + 23) / 10;
    
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
        N += (month > 2) ? 1 : 0;

    // Convert longitude to hour value
    float lngHour = lon / 15.0;

    // Approximate sunrise time
    float t_rise = N + ((6 - lngHour) / 24);
    float t_set = N + ((18 - lngHour) / 24);

    // Sun's mean anomaly
    float M_rise = (0.9856 * t_rise) - 3.289;
    float M_set = (0.9856 * t_set) - 3.289;

    // Sun's true longitude
    float L_rise = fmod(M_rise + (1.916 * sin(M_rise * PI / 180)) +
        (0.020 * sin(2 * M_rise * PI / 180)) + 282.634, 360);
    float L_set = fmod(M_set + (1.916 * sin(M_set * PI / 180)) +
        (0.020 * sin(2 * M_set * PI / 180)) + 282.634, 360);

    // Sun's right ascension
    float RA_rise = fmod(atan(0.91764 * tan(L_rise * PI / 180)) * 180 / PI, 360);
    float RA_set = fmod(atan(0.91764 * tan(L_set * PI / 180)) * 180 / PI, 360);

    // Right ascension value needs to be in the same quadrant as L
    float Lquadrant_rise = floor(L_rise / 90) * 90;
    float RAquadrant_rise = floor(RA_rise / 90) * 90;
    RA_rise = RA_rise + (Lquadrant_rise - RAquadrant_rise);

    float Lquadrant_set = floor(L_set / 90) * 90;
    float RAquadrant_set = floor(RA_set / 90) * 90;
    RA_set = RA_set + (Lquadrant_set - RAquadrant_set);

    // Convert to hours
    RA_rise /= 15;
    RA_set /= 15;

    // Calculate sun's declination
    float sinDec_rise = 0.39782 * sin(L_rise * PI / 180);
    float cosDec_rise = cos(asin(sinDec_rise));

    float sinDec_set = 0.39782 * sin(L_set * PI / 180);
    float cosDec_set = cos(asin(sinDec_set));

    // Calculate sun's local hour angle
    float cosH_rise = (cos(90.833 * PI / 180) - (sinDec_rise * sin(lat * PI / 180))) /
        (cosDec_rise * cos(lat * PI / 180));
    float cosH_set = (cos(90.833 * PI / 180) - (sinDec_set * sin(lat * PI / 180))) /
        (cosDec_set * cos(lat * PI / 180));

    // Check if sun is always up or always down
    if (cosH_rise > 1 || cosH_set > 1)
    {
        // Sun never rises (polar night)
        return { -1, -1, false };
    }

    if (cosH_rise < -1 || cosH_set < -1)
    {
        // Sun never sets (midnight sun)
        return { 0, 24, false };
    }

    // Finish calculating H and convert into hours
    float H_rise = 360 - acos(cosH_rise) * 180 / PI;
    float H_set = acos(cosH_set) * 180 / PI;

    H_rise /= 15;
    H_set /= 15;

    // Calculate local mean time of rising/setting
    float T_rise = H_rise + RA_rise - (0.06571 * t_rise) - 6.622;
    float T_set = H_set + RA_set - (0.06571 * t_set) - 6.622;

    // Adjust back to UTC and then to local time
    float UT_rise = fmod(T_rise - lngHour + 24, 24);
    float UT_set = fmod(T_set - lngHour + 24, 24);

    return { fmodf(UT_rise + tzOffset + 24.0f, 24.0f), fmodf(UT_set + tzOffset + 24.0f, 24.0f), true };
}