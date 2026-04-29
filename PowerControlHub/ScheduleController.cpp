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

#include "Local.h"

#include "ScheduleController.h"
#include "DateTimeManager.h"
#include "SystemFunctions.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ScheduleController::ScheduleController(RelayController* relayController, MessageBus* messageBus)
    : _relayController(relayController),
      _messageBus(messageBus),
      _isDayTime(true),
      _gpsLat(0.0),
      _gpsLon(0.0),
      _hasGps(false),
      _sunriseMinutes(SunTimeUnknown),
      _sunsetMinutes(SunTimeUnknown),
      _sunCalcDay(0),
      _lastCheckMs(0)
{
    for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
    {
        _lastFiredDay[i]       = 0;
        _lastFiredMinute[i]    = NeverFiredMinute;
        _lastIntervalFireMs[i] = 0;
        _pulseStartMs[i]       = 0;
        _pulseDurMs[i]         = 0;
        _pulseRelayIdx[i]      = 0;
        _pulseActive[i]        = false;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ScheduleController::begin()
{
    if (!_messageBus)
        return;

    _messageBus->subscribe<LightSensorUpdated>([this](bool isDayTime, uint16_t lightLevel, uint16_t averageLightLevel)
    {
        (void)lightLevel;
        (void)averageLightLevel;
        _isDayTime = isDayTime;
    });

    _messageBus->subscribe<GpsLocationUpdated>([this](double latitude, double longitude)
    {
        _gpsLat  = latitude;
        _gpsLon  = longitude;
        _hasGps  = true;
        _sunCalcDay = 0; // force sun-time recompute on next update
    });
}

void ScheduleController::update(uint64_t now)
{
    if (!DateTimeManager::isTimeSet())
        return;

    // Pulse ends must be checked every tick for accurate relay-off timing
    updatePulses(now);

    // Throttle event evaluation to once per second
    if (!SystemFunctions::hasElapsed(now, _lastCheckMs, ScheduleCheckIntervalMs))
        return;

    _lastCheckMs = now;

    Config* cfg = ConfigManager::getConfigPtr();

    if (!cfg || !cfg->scheduler.isEnabled)
        return;

    uint8_t hour = DateTimeManager::getHour();
    uint8_t min = DateTimeManager::getMinute();
    uint8_t day = DateTimeManager::getDay();
    uint8_t month = DateTimeManager::getMonth();
    uint16_t year = DateTimeManager::getYear();

    if (year == 0)
        return;

    uint32_t today = encodeDay(year, month, day);
    uint16_t minuteOfDay = static_cast<uint16_t>(hour) * MinutesPerHour + min;

    // Recompute sunrise/sunset when the calendar date changes
    if (today != _sunCalcDay)
    {
        recomputeSunTimes(year, month, day);
        _sunCalcDay = today;
    }

    uint8_t dowBit = calcDayOfWeekBit(year, month, day);

    for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
    {
        const ScheduledEvent& ev = cfg->scheduler.events[i];

        if (!ev.enabled)
            continue;

        if (ev.triggerType == TriggerType::None && ev.actionType == ExecutionActionType::None)
            continue;

        if (!isTriggerDue(i, ev, hour, min, dowBit, day, month, today, now))
            continue;

        // Record that the trigger was seen for this minute, whether or not
        // the condition is met, to prevent repeated checks within the same minute
        if (ev.triggerType != TriggerType::Interval)
        {
            _lastFiredDay[i]    = today;
            _lastFiredMinute[i] = minuteOfDay;
        }

        if (!isConditionMet(ev, hour, min, dowBit))
            continue;

        executeAction(i, ev, now);
    }
}

// ---------------------------------------------------------------------------
// Trigger evaluation
// ---------------------------------------------------------------------------

bool ScheduleController::isTriggerDue(uint8_t idx,
    const ScheduledEvent& ev,
    uint8_t hour, uint8_t minute,
    uint8_t dowBit,
    uint8_t day, uint8_t month,
    uint32_t today,
    uint64_t now)
{
    uint16_t currentMOD = static_cast<uint16_t>(hour) * MinutesPerHour + minute;

    switch (ev.triggerType)
    {
        case TriggerType::TimeOfDay:
        {
            if (hour != ev.triggerPayload[0] || minute != ev.triggerPayload[1])
                return false;

            return _lastFiredDay[idx] != today || _lastFiredMinute[idx] != currentMOD;
        }

        case TriggerType::Sunrise:
        {
            if (_sunriseMinutes == SunTimeUnknown)
                return false;

            int16_t offset    = static_cast<int16_t>(ev.triggerPayload[0]
                                                    | (ev.triggerPayload[1] << 8));
            int16_t fireMOD   = (((_sunriseMinutes + offset) % MinutesPerDay) + MinutesPerDay) % MinutesPerDay;
            
            if (currentMOD != static_cast<uint16_t>(fireMOD))
                return false;

            return _lastFiredDay[idx] != today || _lastFiredMinute[idx] != currentMOD;
        }

        case TriggerType::Sunset:
        {
            if (_sunsetMinutes == SunTimeUnknown)
                return false;

            int16_t offset    = static_cast<int16_t>(ev.triggerPayload[0]
                                                    | (ev.triggerPayload[1] << 8));
            int16_t fireMOD   = (((_sunsetMinutes + offset) % MinutesPerDay) + MinutesPerDay) % MinutesPerDay;

            if (currentMOD != static_cast<uint16_t>(fireMOD))
                return false;

            return _lastFiredDay[idx] != today || _lastFiredMinute[idx] != currentMOD;
        }

        case TriggerType::Interval:
        {
            uint16_t intervalMin = static_cast<uint16_t>(ev.triggerPayload[0]
                                                        | (ev.triggerPayload[1] << 8));
            if (intervalMin == 0)
                return false;

            uint64_t intervalMs = static_cast<uint64_t>(intervalMin) * MsPerMinute;

            if (_lastIntervalFireMs[idx] == 0)
            {
                // First encounter: arm the timer, do not fire yet
                _lastIntervalFireMs[idx] = now;
                return false;
            }

            if (!SystemFunctions::hasElapsed(now, _lastIntervalFireMs[idx], intervalMs))
                return false;

            _lastIntervalFireMs[idx] = now;
            return true;
        }

        case TriggerType::DayOfWeek:
        {
            // Fires at midnight (minute 0) on matching days
            if (currentMOD != 0)
                return false;

            if (!(ev.triggerPayload[0] & dowBit))
                return false;

            return _lastFiredDay[idx] != today || _lastFiredMinute[idx] != 0;
        }

        case TriggerType::Date:
        {
            // Fires at midnight (minute 0) on a specific day/month
            if (currentMOD != 0)
                return false;

            if (day != ev.triggerPayload[0] || month != ev.triggerPayload[1])
                return false;

            return _lastFiredDay[idx] != today || _lastFiredMinute[idx] != 0;
        }

        case TriggerType::None:
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// Condition evaluation
// ---------------------------------------------------------------------------

bool ScheduleController::isConditionMet(const ScheduledEvent& ev,
                                         uint8_t hour, uint8_t minute,
                                         uint8_t dowBit) const
{
    switch (ev.conditionType)
    {
        case ConditionType::None:
            return true;

        case ConditionType::TimeWindow:
        {
            uint16_t current  = static_cast<uint16_t>(hour) * MinutesPerHour + minute;
            uint16_t winStart = static_cast<uint16_t>(ev.conditionPayload[0]) * MinutesPerHour
                              + ev.conditionPayload[1];
            uint16_t winEnd   = static_cast<uint16_t>(ev.conditionPayload[2]) * MinutesPerHour
                              + ev.conditionPayload[3];

            if (winStart <= winEnd)
                return current >= winStart && current <= winEnd;
            else
                return current >= winStart || current <= winEnd; // spans midnight
        }

        case ConditionType::DayOfWeek:
            return (ev.conditionPayload[0] & dowBit) != 0;

        case ConditionType::IsDark:
        {
            if (_sunriseMinutes != SunTimeUnknown && _sunsetMinutes != SunTimeUnknown)
            {
                uint16_t current = static_cast<uint16_t>(hour) * MinutesPerHour + minute;
                return current < static_cast<uint16_t>(_sunriseMinutes)
                    || current > static_cast<uint16_t>(_sunsetMinutes);
            }

            return !_isDayTime;
        }

        case ConditionType::IsDaylight:
        {
            if (_sunriseMinutes != SunTimeUnknown && _sunsetMinutes != SunTimeUnknown)
            {
                uint16_t current = static_cast<uint16_t>(hour) * MinutesPerHour + minute;
                return current >= static_cast<uint16_t>(_sunriseMinutes)
                    && current <= static_cast<uint16_t>(_sunsetMinutes);
            }

            return _isDayTime;
        }

        case ConditionType::RelayIsOn:

            if (!_relayController)
                return false;

            return _relayController->getRelayStatus(ev.conditionPayload[0]).status == 1;

        case ConditionType::RelayIsOff:

            if (!_relayController)
                return false;

            return _relayController->getRelayStatus(ev.conditionPayload[0]).status == 0;

        default:
            return true;
    }
}

// ---------------------------------------------------------------------------
// Action execution
// ---------------------------------------------------------------------------

void ScheduleController::executeAction(uint8_t idx, const ScheduledEvent& ev, uint64_t now)
{
    if (!_relayController)
        return;

    switch (ev.actionType)
    {
        case ExecutionActionType::RelayOn:
            _relayController->setRelayState(ev.actionPayload[0], true);
            break;

        case ExecutionActionType::RelayOff:
            _relayController->setRelayState(ev.actionPayload[0], false);
            break;

        case ExecutionActionType::RelayToggle:
        {
            CommandResult current = _relayController->getRelayStatus(ev.actionPayload[0]);
            _relayController->setRelayState(ev.actionPayload[0], current.status == 0);
            break;
        }

        case ExecutionActionType::RelayPulse:
        {
            uint16_t durationSec = static_cast<uint16_t>(ev.actionPayload[1]
                                                        | (ev.actionPayload[2] << 8));
            if (durationSec == 0)
                break;

            uint8_t relayIdx = ev.actionPayload[0];
            _relayController->setRelayState(relayIdx, true);
            _pulseStartMs[idx]  = now;
            _pulseDurMs[idx]    = static_cast<uint64_t>(durationSec) * MsPerSecond;
            _pulseRelayIdx[idx] = relayIdx;
            _pulseActive[idx]   = true;
            break;
        }

        case ExecutionActionType::AllRelaysOn:
            _relayController->turnAllRelaysOn();
            break;

        case ExecutionActionType::AllRelaysOff:
            _relayController->turnAllRelaysOff();
            break;

        case ExecutionActionType::SetPinLow:
        {
            uint8_t pin = ev.actionPayload[0];
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
            break;
		}

        case ExecutionActionType::SetPinHigh:
        {
            uint8_t pin = ev.actionPayload[0];
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            break;
        }

        case ExecutionActionType::None:
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Pulse management
// ---------------------------------------------------------------------------

void ScheduleController::updatePulses(uint64_t now)
{
    if (!_relayController)
        return;

    for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
    {
        if (!_pulseActive[i])
            continue;

        if (!SystemFunctions::hasElapsed(now, _pulseStartMs[i], _pulseDurMs[i]))
            continue;

        _relayController->setRelayState(_pulseRelayIdx[i], false);
        _pulseActive[i] = false;
    }
}

// ---------------------------------------------------------------------------
// Solar algorithm constants (NOAA simplified model)
// ---------------------------------------------------------------------------

static constexpr long   JulianDayJ2000        = 2451545L;
static constexpr double JulianDayJ2000D       = 2451545.0;
static constexpr long   JulianDayFormulaBias  = 32045L;
static constexpr double JdNoonOffset          = 0.5;    // JD 0.0 = noon; midnight = JDN - 0.5
static constexpr double MeanNoonCorrection    = 0.0008;
static constexpr double SolarMeanAnomalyBase  = 357.5291;
static constexpr double SolarMeanAnomalyRate  = 0.98560028;
static constexpr double EqCenterC1            = 1.9148;
static constexpr double EqCenterC2            = 0.0200;
static constexpr double EqCenterC3            = 0.0003;
static constexpr double LongitudeOfPerihelion = 102.9372;
static constexpr double TransitCorrSin1       = 0.0053;
static constexpr double TransitCorrSin2       = 0.0069;
static constexpr double EarthAxialTilt        = 23.4397;  // degrees
static constexpr double SolarHorizonElevation = -0.8333; // degrees; accounts for refraction + disc
static constexpr double DegreesPerCircle      = 360.0;
static constexpr double DegreesPerHalfCircle  = 180.0;

// ---------------------------------------------------------------------------
// Sun time computation
// ---------------------------------------------------------------------------

void ScheduleController::recomputeSunTimes(uint16_t year, uint8_t month, uint8_t day)
{
    if (!_hasGps)
    {
        _sunriseMinutes = SunTimeUnknown;
        _sunsetMinutes  = SunTimeUnknown;
        return;
    }

    calcSunTimes(_gpsLat, _gpsLon, year, month, day, _sunriseMinutes, _sunsetMinutes);
}

void ScheduleController::calcSunTimes(double lat, double lon,
                                       uint16_t year, uint8_t month, uint8_t day,
                                       int16_t& sunriseMin, int16_t& sunsetMin)
{
    // NOAA simplified solar algorithm
    // Step 1: Julian Day Number (Gregorian calendar)
    int a      = (14 - month) / 12;
    int y      = year + 4800 - a;
    int m      = month + 12 * a - 3;
    long JDN   = static_cast<long>(day)
               + (153L * m + 2L) / 5L
               + 365L * y
               + y / 4 - y / 100 + y / 400
               - JulianDayFormulaBias;

    // Step 2: Mean solar noon
    double n      = static_cast<double>(JDN - JulianDayJ2000) + MeanNoonCorrection;
    double Jstar  = n - lon / DegreesPerCircle;

    // Step 3: Solar mean anomaly (degrees → radians for trig calls)
    double M_deg  = fmod(SolarMeanAnomalyBase + SolarMeanAnomalyRate * Jstar, DegreesPerCircle);
    double M_rad  = M_deg * M_PI / DegreesPerHalfCircle;

    // Step 4: Equation of the center
    double C = EqCenterC1 * sin(M_rad)
             + EqCenterC2 * sin(2.0 * M_rad)
             + EqCenterC3 * sin(3.0 * M_rad);

    // Step 5: Ecliptic longitude
    double lambda     = fmod(M_deg + C + DegreesPerHalfCircle + LongitudeOfPerihelion, DegreesPerCircle);
    double lambda_rad = lambda * M_PI / DegreesPerHalfCircle;

    // Step 6: Solar transit
    double Jtransit = JulianDayJ2000D + Jstar
                    + TransitCorrSin1 * sin(M_rad)
                    - TransitCorrSin2 * sin(2.0 * lambda_rad);

    // Step 7: Declination
    double sin_delta = sin(lambda_rad) * sin(EarthAxialTilt * M_PI / DegreesPerHalfCircle);
    double cos_delta = cos(asin(sin_delta));

    // Step 8: Hour angle at SolarHorizonElevation (accounts for refraction + solar disc)
    double lat_rad   = lat * M_PI / DegreesPerHalfCircle;
    double cos_omega = (sin(SolarHorizonElevation * M_PI / DegreesPerHalfCircle) - sin(lat_rad) * sin_delta)
                     / (cos(lat_rad) * cos_delta);

    if (cos_omega < -1.0 || cos_omega > 1.0)
    {
        // Polar day (midnight sun) or polar night — sun times unavailable
        sunriseMin = SunTimeUnknown;
        sunsetMin  = SunTimeUnknown;
        return;
    }

    double omega_deg = acos(cos_omega) * DegreesPerHalfCircle / M_PI;

    // Step 9: Julian dates of sunrise and sunset
    double Jrise = Jtransit - omega_deg / DegreesPerCircle;
    double Jset  = Jtransit + omega_deg / DegreesPerCircle;

    // Step 10: Convert Julian date to UTC minutes from midnight on the given date
    // JD fraction 0.0 = noon, so midnight = JDN - JdNoonOffset
    double riseUtcMin = (Jrise - static_cast<double>(JDN) + JdNoonOffset) * MinutesPerDay;
    double setUtcMin  = (Jset  - static_cast<double>(JDN) + JdNoonOffset) * MinutesPerDay;

    // Step 11: Apply timezone offset
    double tzOffsetMin = static_cast<double>(DateTimeManager::getTimezoneOffset()) * MinutesPerHour;
    riseUtcMin += tzOffsetMin;
    setUtcMin  += tzOffsetMin;

    // Wrap to [0, MinutesPerDay)
    int riseRaw = static_cast<int>(riseUtcMin);
    int setRaw  = static_cast<int>(setUtcMin);
    sunriseMin  = static_cast<int16_t>(((riseRaw % MinutesPerDay) + MinutesPerDay) % MinutesPerDay);
    sunsetMin   = static_cast<int16_t>(((setRaw  % MinutesPerDay) + MinutesPerDay) % MinutesPerDay);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint8_t ScheduleController::calcDayOfWeekBit(uint16_t year, uint8_t month, uint8_t day)
{
    // Sakamoto's algorithm — returns 0=Sun, 1=Mon, …, 6=Sat
    static const uint8_t t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    uint16_t y = year;

    if (month < 3)
        y--;

    uint8_t dow = static_cast<uint8_t>(
        (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7);

    // Convert to bitmask bit: bit0=Mon … bit6=Sun
    // dow 0=Sun→bit6, 1=Mon→bit0, …, 6=Sat→bit5
    return (dow == 0) ? 64u : static_cast<uint8_t>(1u << (dow - 1));
}

uint32_t ScheduleController::encodeDay(uint16_t year, uint8_t month, uint8_t day)
{
    return static_cast<uint32_t>(year) * 10000u
         + static_cast<uint32_t>(month) * 100u
         + day;
}
