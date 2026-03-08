#pragma once

#include "Config.h"

#if defined(SCHEDULER_SUPPORT)

#include "RelayController.h"
#include "MessageBus.h"
#include "ConfigManager.h"

constexpr int16_t       SunTimeUnknown         = -1;
constexpr unsigned long ScheduleCheckIntervalMs = 1000UL;

constexpr uint16_t      MinutesPerDay    = 1440u;
constexpr uint16_t      MinutesPerHour   = 60u;
constexpr unsigned long MsPerMinute      = 60000UL;
constexpr unsigned long MsPerSecond      = 1000UL;
constexpr uint16_t      NeverFiredMinute = 0xFFFFu;

class ScheduleController
{
private:
    RelayController* _relayController;
    MessageBus*      _messageBus;

    // Per-event runtime state (not persisted — reset on boot)
    uint32_t      _lastFiredDay[ConfigMaxScheduledEvents];       // packed YYYYMMDD of last fire
    uint16_t      _lastFiredMinute[ConfigMaxScheduledEvents];    // minute-of-day (0-1439) of last fire
    unsigned long _lastIntervalFireMs[ConfigMaxScheduledEvents]; // millis() of last interval fire
    unsigned long _pulseStartMs[ConfigMaxScheduledEvents];       // millis() when pulse started
    unsigned long _pulseDurMs[ConfigMaxScheduledEvents];         // pulse duration in ms
    uint8_t       _pulseRelayIdx[ConfigMaxScheduledEvents];      // relay index for active pulse
    bool          _pulseActive[ConfigMaxScheduledEvents];        // true while pulse relay is on

    // Light / position state (updated via MessageBus)
    bool    _isDayTime;
    double  _gpsLat;
    double  _gpsLon;
    bool    _hasGps;
    int16_t _sunriseMinutes; // local minutes from midnight, SunTimeUnknown if unavailable
    int16_t _sunsetMinutes;  // local minutes from midnight, SunTimeUnknown if unavailable
    uint32_t _sunCalcDay;    // packed YYYYMMDD for which sun times were computed

    unsigned long _lastCheckMs;

    // Returns true when the trigger for event idx is due to fire right now
    bool isTriggerDue(uint8_t idx,
                      const ScheduledEvent& ev,
                      uint8_t hour, uint8_t minute,
                      uint8_t dowBit,
                      uint8_t day, uint8_t month,
                      uint32_t today,
                      unsigned long nowMs);

    // Returns true when all conditions for the event are satisfied
    bool isConditionMet(const ScheduledEvent& ev,
                        uint8_t hour, uint8_t minute,
                        uint8_t dowBit) const;

    // Executes the action associated with an event
    void executeAction(uint8_t idx, const ScheduledEvent& ev, unsigned long nowMs);

    // Turns off any relay pulses whose duration has expired
    void updatePulses(unsigned long nowMs);

    // Recomputes sunrise/sunset minute-of-day values for the given date
    void recomputeSunTimes(uint16_t year, uint8_t month, uint8_t day);

    // NOAA simplified solar calculation; returns local minutes from midnight
    static void calcSunTimes(double lat, double lon,
                             uint16_t year, uint8_t month, uint8_t day,
                             int16_t& sunriseMin, int16_t& sunsetMin);

    // Returns the day-of-week bitmask bit for a date (bit0=Mon .. bit6=Sun)
    static uint8_t calcDayOfWeekBit(uint16_t year, uint8_t month, uint8_t day);

    // Packs year/month/day into a uint32 YYYYMMDD key
    static uint32_t encodeDay(uint16_t year, uint8_t month, uint8_t day);

public:
    ScheduleController(RelayController* relayController, MessageBus* messageBus);

    // Subscribe to MessageBus events — call once during setup
    void begin();

    // Called every loop iteration; nowMs = millis()
    void update(unsigned long nowMs);
};

#endif
