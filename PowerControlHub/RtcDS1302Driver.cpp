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
#include "RtcDS1302Driver.h"
#include "Local.h"
#include "SystemDefinitions.h"
#include "ConfigManager.h"

#include <Ds1302.h>

static Ds1302* rtc = nullptr;

RtcDS1302Driver::RtcDS1302Driver()
    : _available(false)
{
}

RtcDS1302Driver::~RtcDS1302Driver()
{
    delete rtc;
    rtc = nullptr;
}

bool RtcDS1302Driver::begin()
{
    // Attempt to initialise using runtime configuration if available.
    Config* cfg = ConfigManager::getConfigPtr();
    if (cfg == nullptr)
    {
        _available = false;
        return false;
    }

    // If pins are not configured, report unavailable rather than silently disabling.
    if (cfg->rtc.dataPin == PinDisabled || cfg->rtc.clockPin == PinDisabled || cfg->rtc.resetPin == PinDisabled)
    {
        _available = false;
        return false;
    }

    // Delegate to the pin-overload which performs full initialisation.
    return begin(cfg->rtc.dataPin, cfg->rtc.clockPin, cfg->rtc.resetPin);
}

bool RtcDS1302Driver::begin(uint8_t dataPin, uint8_t clockPin, uint8_t resetPin)
{
    if (dataPin == PinDisabled || clockPin == PinDisabled || resetPin == PinDisabled)
    {
        _available = false;
        return false;
    }
    delete rtc;
    rtc = new Ds1302(resetPin, clockPin, dataPin);
    rtc->init();

    // Ensure oscillator is running
    if (rtc->isHalted())
    {
        rtc->start();
    }

    // Attempt to read current time. The DS1302 stores a 2-digit year (0-99).
    Ds1302::DateTime dt;
    rtc->getDateTime(&dt);
    uint16_t fullYear = 2000 + static_cast<uint16_t>(dt.year);

    // If year is clearly invalid, set a sensible default (matches previous behaviour)
    if (fullYear < 2025)
    {
        Ds1302::DateTime defaultDt;
        defaultDt.year = static_cast<uint8_t>(2025 - 2000);
        defaultDt.month = 1;
        defaultDt.day = 1;
        defaultDt.hour = 0;
        defaultDt.minute = 0;
        defaultDt.second = 0;
        defaultDt.dow = 0;
        rtc->setDateTime(&defaultDt);
        fullYear = 2025;
    }

    (void)fullYear; // keep variable for readability; availability determined by successful read
    _available = true;
    return _available;
}

bool RtcDS1302Driver::isAvailable() const
{
    return _available;
}

unsigned long RtcDS1302Driver::readTimestamp()
{
    if (!_available)
        return 0;

    if (!rtc)
        return 0;

    Ds1302::DateTime rtcTime;
    rtc->getDateTime(&rtcTime);

    // DS1302 returns 2-digit year (0..99) — convert to full year in 2000s
    uint16_t year = 2000 + static_cast<uint16_t>(rtcTime.year);
    uint8_t month = rtcTime.month;
    uint8_t day = rtcTime.day;
    uint8_t hour = rtcTime.hour;
    uint8_t minute = rtcTime.minute;
    uint8_t second = rtcTime.second;

    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    unsigned long days = 0;

    for (uint16_t y = 1970; y < year; y++)
    {
        days += 365;
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
            days++;
    }

    for (uint8_t m = 1; m < month; m++)
    {
        days += daysInMonth[m - 1];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
            days++;
    }

    days += day - 1;

    unsigned long timestamp = days * 86400UL;
    timestamp += hour   * 3600UL;
    timestamp += minute * 60UL;
    timestamp += second;

    return timestamp;
}

bool RtcDS1302Driver::writeTimestamp(unsigned long unixTimestamp)
{
    if (!_available)
        return false;

    if (!rtc)
        return false;

    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    uint8_t       second = unixTimestamp % 60;
    unixTimestamp /= 60;
    uint8_t       minute = unixTimestamp % 60;
    unixTimestamp /= 60;
    uint8_t       hour   = unixTimestamp % 24;
    unsigned long days   = unixTimestamp / 24;

    uint16_t year = 1970;
    while (true)
    {
        uint16_t daysInYear = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
        if (days < daysInYear)
            break;
        days -= daysInYear;
        year++;
    }

    bool    isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    uint8_t month      = 1;

    while (month <= 12)
    {
        uint8_t monthDays = daysInMonth[month - 1];
        if (month == 2 && isLeapYear)
            monthDays = 29;
        if (days < monthDays)
            break;
        days -= monthDays;
        month++;
    }

    uint8_t day = static_cast<uint8_t>(days) + 1;

    Ds1302::DateTime rtcTime;
    rtcTime.year = static_cast<uint8_t>(year - 2000);
    rtcTime.month = month;
    rtcTime.day = day;
    rtcTime.hour = hour;
    rtcTime.minute = minute;
    rtcTime.second = second;
    rtcTime.dow = 0;
    rtc->setDateTime(&rtcTime);
    return true;
}

bool RtcDS1302Driver::runDiagnostics(char* buffer, const uint8_t bufferLength)
{
    if (!_available)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not available"));
        return false;
    }

    if (!rtc)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not available"));
        return false;
    }

    Ds1302::DateTime currentTime;
    rtc->getDateTime(&currentTime);

    // Convert 2-digit year to full year
    uint16_t year = 2000 + static_cast<uint16_t>(currentTime.year);

    if (rtc->isHalted())
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not running"));
        return false;
    }

    if (year < 2025 || year > 2099)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC time out of range: %d"), year);
        return false;
    }

    snprintf_P(buffer, bufferLength, PSTR("RTC OK: %04d-%02d-%02d %02d:%02d:%02d"),
        year, currentTime.month, currentTime.day,
        currentTime.hour, currentTime.minute, currentTime.second);
    return true;
}
