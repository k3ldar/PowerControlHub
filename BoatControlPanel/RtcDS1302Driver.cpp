#include "RtcDS1302Driver.h"
#include "Local.h"

#if defined(BOAT_CONTROL_PANEL)
#include "BoatControlPanelConstants.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>

static ThreeWire rtcWire(PinRtcData, PinRtcClock, PinRtcReset);
static RtcDS1302<ThreeWire> rtc(rtcWire);
#endif

constexpr unsigned long DefaultTimestamp = 1735689600UL;

RtcDS1302Driver::RtcDS1302Driver()
    : _available(false)
{
}

RtcDS1302Driver::~RtcDS1302Driver()
{
}

bool RtcDS1302Driver::begin()
{
#if defined(BOAT_CONTROL_PANEL)
    Serial.println(F("Initializing DS1302 RTC..."));
    
    rtc.Begin();
    rtc.SetIsWriteProtected(false);

    if (!rtc.IsDateTimeValid())
    {
        Serial.println(F("RTC date/time is invalid. Setting to default time."));
        
        RtcDateTime defaultTime(2025, 1, 1, 0, 0, 0);
        rtc.SetDateTime(defaultTime);
    }

    if (!rtc.GetIsRunning())
    {
        Serial.println(F("RTC was not running. Starting clock."));
        rtc.SetIsRunning(true);
    }

    _available = rtc.IsDateTimeValid();
    
    if (_available)
    {
        Serial.println(F("DS1302 RTC initialized successfully."));
    }
    else
    {
        Serial.println(F("DS1302 RTC initialization failed."));
    }
    
    return _available;
#else
    Serial.println(F("DS1302 RTC not compiled for this platform."));
    return false;
#endif
}

bool RtcDS1302Driver::isAvailable() const
{
    return _available;
}

unsigned long RtcDS1302Driver::readTimestamp()
{
#if defined(BOAT_CONTROL_PANEL)
    if (!_available)
    {
        return 0;
    }

    RtcDateTime rtcTime = rtc.GetDateTime();

    if (!rtcTime.IsValid())
    {
        Serial.println(F("RTC time is invalid during read."));
        return 0;
    }

    uint16_t year = rtcTime.Year();
    uint8_t month = rtcTime.Month();
    uint8_t day = rtcTime.Day();
    uint8_t hour = rtcTime.Hour();
    uint8_t minute = rtcTime.Minute();
    uint8_t second = rtcTime.Second();

    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    unsigned long days = 0;
    
    for (uint16_t y = 1970; y < year; y++)
    {
        days += 365;
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
        {
            days++;
        }
    }
    
    for (uint8_t m = 1; m < month; m++)
    {
        days += daysInMonth[m - 1];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        {
            days++;
        }
    }
    
    days += day - 1;
    
    unsigned long timestamp = days * 86400UL;
    timestamp += hour * 3600UL;
    timestamp += minute * 60UL;
    timestamp += second;
    
    return timestamp;
#else
    return 0;
#endif
}

bool RtcDS1302Driver::writeTimestamp(unsigned long unixTimestamp)
{
#if defined(BOAT_CONTROL_PANEL)
    if (!_available)
    {
        Serial.println(F("RTC not available. Cannot write timestamp."));
        return false;
    }

    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    uint8_t second = unixTimestamp % 60;
    unixTimestamp /= 60;
    uint8_t minute = unixTimestamp % 60;
    unixTimestamp /= 60;
    uint8_t hour = unixTimestamp % 24;
    unsigned long days = unixTimestamp / 24;
    
    uint16_t year = 1970;
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
    
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    
    uint8_t month = 1;
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
    
    uint8_t day = days + 1;

    Serial.print(F("Writing time to RTC: "));
    Serial.print(year);
    Serial.print(F("-"));
    Serial.print(month);
    Serial.print(F("-"));
    Serial.print(day);
    Serial.print(F(" "));
    Serial.print(hour);
    Serial.print(F(":"));
    Serial.print(minute);
    Serial.print(F(":"));
    Serial.println(second);

    RtcDateTime rtcTime(year, month, day, hour, minute, second);
    rtc.SetDateTime(rtcTime);
    
    return true;
#else
    (void)unixTimestamp;
    return false;
#endif
}

bool RtcDS1302Driver::runDiagnostics(char* buffer, const uint8_t bufferLength)
{
#if defined(BOAT_CONTROL_PANEL)
    if (!_available)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not available"));
        return false;
    }

    RtcDateTime currentTime = rtc.GetDateTime();
    
    if (!currentTime.IsValid())
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC time invalid"));
        return false;
    }

    bool isRunning = rtc.GetIsRunning();
    if (!isRunning)
    {
        snprintf_P(buffer, bufferLength, PSTR("RTC not running"));
        return false;
    }

    bool isWriteProtected = rtc.GetIsWriteProtected();
    if (isWriteProtected)
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
#else
    snprintf_P(buffer, bufferLength, PSTR("RTC not compiled"));
    return false;
#endif
}
