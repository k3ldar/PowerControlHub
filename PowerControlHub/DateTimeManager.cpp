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
#include "DateTimeManager.h"

constexpr uint64_t DefaultTimestamp = 1735689600UL;

RtcDS1302Driver DateTimeManager::_rtcDriver;

uint64_t DateTimeManager::_syncedTimestamp = 0;
uint64_t DateTimeManager::_syncedMillis = 0;
bool DateTimeManager::_isSet = false;
int8_t DateTimeManager::_timezoneOffset = 0;

void DateTimeManager::begin(const RtcConfig& rtcConfig)
{
	if (rtcConfig.dataPin != PinDisabled &&
		rtcConfig.clockPin != PinDisabled &&
		rtcConfig.resetPin != PinDisabled)
	{
		_rtcDriver.begin(rtcConfig.dataPin, rtcConfig.clockPin, rtcConfig.resetPin);

		if (_rtcDriver.isAvailable())
		{
			uint64_t rtcTimestamp = _rtcDriver.readTimestamp();
			if (rtcTimestamp > 0)
			{
				setDateTime(rtcTimestamp);
				return;
			}
		}
	}

	setDateTime(DefaultTimestamp);
}

void DateTimeManager::setDateTime()
{
    // Set to default: January 1, 2025 00:00:00
    setDateTime(DefaultTimestamp);
}

void DateTimeManager::setDateTime(uint64_t unixTimestamp)
{
    _syncedTimestamp = unixTimestamp;
    _syncedMillis = SystemFunctions::millis64();
    _isSet = true;

    if (_rtcDriver.isAvailable())
    {
        _rtcDriver.writeTimestamp(unixTimestamp);
    }
}

bool DateTimeManager::setDateTimeISO(const char* isoDateTime)
{
    // Expected format: YYYY-MM-DDTHH:MM:SS (19 characters minimum)
    if (strlen(isoDateTime) < 19)
    {
        return false;
    }

    // Parse components directly
    uint16_t year = (isoDateTime[0] - '0') * 1000 + 
                    (isoDateTime[1] - '0') * 100 + 
                    (isoDateTime[2] - '0') * 10 + 
                    (isoDateTime[3] - '0');
                    
    uint8_t month = (isoDateTime[5] - '0') * 10 + (isoDateTime[6] - '0');
    uint8_t day = (isoDateTime[8] - '0') * 10 + (isoDateTime[9] - '0');
    uint8_t hour = (isoDateTime[11] - '0') * 10 + (isoDateTime[12] - '0');
    uint8_t minute = (isoDateTime[14] - '0') * 10 + (isoDateTime[15] - '0');
    uint8_t second = (isoDateTime[17] - '0') * 10 + (isoDateTime[18] - '0');

    // Basic validation
    if (year < 2000 || year > 2099 || 
        month < 1 || month > 12 || 
        day < 1 || day > 31 ||
        hour > 23 ||
        minute > 59 ||
        second > 59)
    {
        return false;
    }

    // Convert to Unix timestamp and store
    uint64_t timestamp = dateTimeToUnix(year, month, day, hour, minute, second);
    setDateTime(timestamp);
    return true;
}

uint64_t DateTimeManager::getCurrentTime()
{
    if (!_isSet)
    {
        return 0;
    }

    uint64_t elapsedMillis = SystemFunctions::millis64() - _syncedMillis;
    uint64_t elapsedSeconds = elapsedMillis / 1000;
    uint64_t utcTime = _syncedTimestamp + elapsedSeconds;

    int32_t offsetSeconds = static_cast<int32_t>(_timezoneOffset) * 3600;
    if (offsetSeconds >= 0)
        return utcTime + static_cast<uint64_t>(offsetSeconds);
    else
        return utcTime - static_cast<uint64_t>(-offsetSeconds);
}

bool DateTimeManager::isTimeSet()
{
    return _isSet;
}

uint64_t DateTimeManager::getSecondsSinceSync()
{
    if (!_isSet)
    {
        return 0;
    }

    uint64_t elapsedMillis = SystemFunctions::millis64() - _syncedMillis;
    return elapsedMillis / 1000;
}

uint16_t DateTimeManager::getYear()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return year;
}

uint8_t DateTimeManager::getMonth()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return month;
}

uint8_t DateTimeManager::getDay()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return day;
}

uint8_t DateTimeManager::getHour()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return hour;
}

uint8_t DateTimeManager::getMinute()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return minute;
}

uint8_t DateTimeManager::getSecond()
{
    if (!_isSet)
    {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return second;
}

bool DateTimeManager::formatDateTime(char* buffer, const uint8_t bufferLength)
{
    if (!_isSet)
    {
        return false;
    }

    uint64_t currentTime = getCurrentTime();
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(currentTime, year, month, day, hour, minute, second);

    snprintf_P(buffer, bufferLength, PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
             year, month, day, hour, minute, second);
    return true;
}

bool DateTimeManager::formatDateTimeReadable(char* buffer, const uint8_t bufferLength)
{
    if (!_isSet)
    {
        return false;
    }

    uint64_t currentTime = getCurrentTime();
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(currentTime, year, month, day, hour, minute, second);

    snprintf_P(buffer, bufferLength, PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
             year, month, day, hour, minute, second);
    return true;
}

void DateTimeManager::reset()
{
    _syncedTimestamp = 0;
    _syncedMillis = 0;
    _isSet = false;
}

void DateTimeManager::setTimezoneOffset(int8_t offsetHours)
{
    if (offsetHours >= -12 && offsetHours <= 14)
    {
        _timezoneOffset = offsetHours;
    }
}

int8_t DateTimeManager::getTimezoneOffset()
{
    return _timezoneOffset;
}

uint64_t DateTimeManager::dateTimeToUnix(uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second)
{
    // Simple Unix timestamp calculation (no timezone, no leap seconds)
    // Days in each month (non-leap year)
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Calculate days since epoch (Jan 1, 1970)
    uint64_t days = 0;
    
    // Years
    for (uint16_t y = 1970; y < year; y++)
    {
        days += 365;
        // Add leap day if leap year
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
        {
            days++;
        }
    }
    
    // Months
    for (uint8_t m = 1; m < month; m++)
    {
        days += daysInMonth[m - 1];
        // Add leap day for February in leap year
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        {
            days++;
        }
    }
    
    // Days
    days += day - 1;
    
    // Convert to seconds and add time components
    uint64_t timestamp = days * 86400UL;  // 86400 seconds per day
    timestamp += hour * 3600UL;
    timestamp += minute * 60UL;
    timestamp += second;
    
    return timestamp;
}

void DateTimeManager::unixToDateTime(uint64_t unixTime, uint16_t& year, uint8_t& month, uint8_t& day,
    uint8_t& hour, uint8_t& minute, uint8_t& second)
{
    // Extract time components
    second = unixTime % 60;
    unixTime /= 60;
    minute = unixTime % 60;
    unixTime /= 60;
    hour = unixTime % 24;
    uint64_t days = unixTime / 24;
    
    // Calculate year
    year = 1970;
    while (true)
    {
        uint16_t daysInYear = 365;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        {
            daysInYear = 366;
        }
        
        if (days < daysInYear)
        {
            break;
        }
        
        days -= daysInYear;
        year++;
    }
    
    // Calculate month and day
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    
    month = 1;
    while (month <= 12)
    {
        uint8_t monthDays = daysInMonth[month - 1];
        if (month == 2 && isLeapYear)
        {
            monthDays = 29;
        }
        
        if (days < monthDays)
        {
            break;
        }
        
        days -= monthDays;
        month++;
    }
    
	day = days + 1;
}

bool DateTimeManager::isRtcAvailable()
{
	return _rtcDriver.isAvailable();
}

bool DateTimeManager::rtcDiagnostic(char* buffer, const uint8_t bufferLength)
{
	return _rtcDriver.runDiagnostics(buffer, bufferLength);
}
