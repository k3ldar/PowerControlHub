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
#include "RtcDS1302Driver.h"
#include "Local.h"
#include "SystemDefinitions.h"
#include "ConfigManager.h"

#include <ThreeWire.h>
#include <RtcDS1302.h>

static ThreeWire* rtcWire = nullptr;
static RtcDS1302<ThreeWire>* rtc = nullptr;

RtcDS1302Driver::RtcDS1302Driver()
    : _available(false)
{
}

RtcDS1302Driver::~RtcDS1302Driver()
{
    delete rtc;
    delete rtcWire;
    rtc = nullptr;
    rtcWire = nullptr;
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
    // Runtime pin check is the sole availability gate — no compile flag required
    if (dataPin == PinDisabled || clockPin == PinDisabled || resetPin == PinDisabled)
    {
        _available = false;
        return false;
    }

    delete rtc;
    delete rtcWire;
    rtcWire = new ThreeWire(dataPin, clockPin, resetPin);
    rtc = new RtcDS1302<ThreeWire>(*rtcWire);

    rtc->Begin();
    rtc->SetIsWriteProtected(false);

    if (!rtc->IsDateTimeValid())
    {
        RtcDateTime defaultTime(2025, 1, 1, 0, 0, 0);
        rtc->SetDateTime(defaultTime);
    }

    if (!rtc->GetIsRunning())
    {
        rtc->SetIsRunning(true);
    }

    _available = rtc->IsDateTimeValid();
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

    RtcDateTime rtcTime = rtc->GetDateTime();

    if (!rtcTime.IsValid())
        return 0;

    uint16_t year   = rtcTime.Year();
    uint8_t  month  = rtcTime.Month();
    uint8_t  day    = rtcTime.Day();
    uint8_t  hour   = rtcTime.Hour();
    uint8_t  minute = rtcTime.Minute();
    uint8_t  second = rtcTime.Second();

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

    RtcDateTime rtcTime(year, month, day, hour, minute, second);
    rtc->SetDateTime(rtcTime);
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

    RtcDateTime currentTime = rtc->GetDateTime();

    if (!currentTime.IsValid())
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC time invalid"));
        return false;
    }

    if (!rtc->GetIsRunning())
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not running"));
        return false;
    }

    if (rtc->GetIsWriteProtected())
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC write protected"));
        return false;
    }

    uint16_t year = currentTime.Year();
    if (year < 2025 || year > 2099)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC time out of range: %d"), year);
        return false;
    }

    snprintf_P(buffer, bufferLength, PSTR("RTC OK: %04d-%02d-%02d %02d:%02d:%02d"),
        currentTime.Year(), currentTime.Month(), currentTime.Day(),
        currentTime.Hour(), currentTime.Minute(), currentTime.Second());
    return true;
}
