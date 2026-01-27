#include "DateTimeManager.h"

constexpr unsigned long DefaultTimestamp = 1735689600UL;

// Static member initialization
unsigned long DateTimeManager::_syncedTimestamp = 0;
unsigned long DateTimeManager::_syncedMillis = 0;
bool DateTimeManager::_isSet = false;

void DateTimeManager::setDateTime() {
    // Set to default: January 1, 2025 00:00:00
    setDateTime(DefaultTimestamp);
}

void DateTimeManager::setDateTime(unsigned long unixTimestamp) {
    _syncedTimestamp = unixTimestamp;
    _syncedMillis = millis();
    _isSet = true;
}

bool DateTimeManager::setDateTimeISO(const char* isoDateTime) {
    // Expected format: YYYY-MM-DDTHH:MM:SS (19 characters minimum)
    if (strlen(isoDateTime) < 19) {
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
        second > 59) {
        return false;
    }

    // Convert to Unix timestamp and store
    unsigned long timestamp = dateTimeToUnix(year, month, day, hour, minute, second);
    setDateTime(timestamp);
    return true;
}

unsigned long DateTimeManager::getCurrentTime() {
    if (!_isSet) {
        return 0;
    }

    // Calculate elapsed time since sync (handle millis() overflow)
    unsigned long currentMillis = millis();
    unsigned long elapsedMillis;
    
    if (currentMillis >= _syncedMillis) {
        elapsedMillis = currentMillis - _syncedMillis;
    } else {
        // millis() has overflowed (after ~49.7 days)
        elapsedMillis = (0xFFFFFFFF - _syncedMillis) + currentMillis + 1;
    }

    // Convert milliseconds to seconds and add to synced timestamp
    unsigned long elapsedSeconds = elapsedMillis / 1000;
    return _syncedTimestamp + elapsedSeconds;
}

bool DateTimeManager::isTimeSet() {
    return _isSet;
}

unsigned long DateTimeManager::getSecondsSinceSync()
{
    if (!_isSet) {
        return 0;
    }

    unsigned long currentMillis = millis();
    unsigned long elapsedMillis;
    
    if (currentMillis >= _syncedMillis) {
        elapsedMillis = currentMillis - _syncedMillis;
    } else {
        elapsedMillis = (0xFFFFFFFF - _syncedMillis) + currentMillis + 1;
    }

    return elapsedMillis / 1000;
}

uint16_t DateTimeManager::getYear() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return year;
}

uint8_t DateTimeManager::getMonth() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return month;
}

uint8_t DateTimeManager::getDay() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return day;
}

uint8_t DateTimeManager::getHour() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return hour;
}

uint8_t DateTimeManager::getMinute() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return minute;
}

uint8_t DateTimeManager::getSecond() {
    if (!_isSet) {
        return 0;
    }

    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(getCurrentTime(), year, month, day, hour, minute, second);
    return second;
}

bool DateTimeManager::formatDateTime(char* buffer, const uint8_t bufferLength)
{
    if (!_isSet) {
        return false;
    }

    unsigned long currentTime = getCurrentTime();
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(currentTime, year, month, day, hour, minute, second);

    snprintf(buffer, bufferLength, "%04d-%02d-%02dT%02d:%02d:%02d",
             year, month, day, hour, minute, second);
    return true;
}

bool DateTimeManager::formatDateTimeReadable(char* buffer, const uint8_t bufferLength)
{
    if (!_isSet)
    {
        return false;
    }

    unsigned long currentTime = getCurrentTime();
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    unixToDateTime(currentTime, year, month, day, hour, minute, second);

    snprintf(buffer, bufferLength, "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);
    return true;
}

void DateTimeManager::reset()
{
    _syncedTimestamp = 0;
    _syncedMillis = 0;
    _isSet = false;
}

unsigned long DateTimeManager::dateTimeToUnix(uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second)
{
    // Simple Unix timestamp calculation (no timezone, no leap seconds)
    // Days in each month (non-leap year)
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Calculate days since epoch (Jan 1, 1970)
    unsigned long days = 0;
    
    // Years
    for (uint16_t y = 1970; y < year; y++) {
        days += 365;
        // Add leap day if leap year
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            days++;
        }
    }
    
    // Months
    for (uint8_t m = 1; m < month; m++) {
        days += daysInMonth[m - 1];
        // Add leap day for February in leap year
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            days++;
        }
    }
    
    // Days
    days += day - 1;
    
    // Convert to seconds and add time components
    unsigned long timestamp = days * 86400UL;  // 86400 seconds per day
    timestamp += hour * 3600UL;
    timestamp += minute * 60UL;
    timestamp += second;
    
    return timestamp;
}

void DateTimeManager::unixToDateTime(unsigned long unixTime, uint16_t& year, uint8_t& month, uint8_t& day,
    uint8_t& hour, uint8_t& minute, uint8_t& second) {
    // Extract time components
    second = unixTime % 60;
    unixTime /= 60;
    minute = unixTime % 60;
    unixTime /= 60;
    hour = unixTime % 24;
    unsigned long days = unixTime / 24;
    
    // Calculate year
    year = 1970;
    while (true) {
        uint16_t daysInYear = 365;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInYear = 366;
        }
        
        if (days < daysInYear) {
            break;
        }
        
        days -= daysInYear;
        year++;
    }
    
    // Calculate month and day
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    
    month = 1;
    while (month <= 12) {
        uint8_t monthDays = daysInMonth[month - 1];
        if (month == 2 && isLeapYear) {
            monthDays = 29;
        }
        
        if (days < monthDays) {
            break;
        }
        
        days -= monthDays;
        month++;
    }
    
    day = days + 1;
}


