#pragma once

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <math.h>

enum class MoonPhase {
    NewMoon = 0,
    WaxingCrescent,
    FirstQuarter,
    WaxingGibbous,
    FullMoon,
    WaningGibbous,
    LastQuarter,
    WaningCrescent
};

// Buffer sizes for moon phase strings with a little extra space
constexpr uint8_t BufferSizeMoonPhaseName = 32;
constexpr uint8_t BufferSizeMoonPhaseDescription = 128;
constexpr uint8_t BufferSizeMoonPhaseSeaDescription = 64;

// Moon phase names, minimum size is 16 bytes to fit longest string
static const char moon_phase_0[] PROGMEM = "New Moon";
static const char moon_phase_1[] PROGMEM = "Waxing Crescent";
static const char moon_phase_2[] PROGMEM = "First Quarter";
static const char moon_phase_3[] PROGMEM = "Waxing Gibbous";
static const char moon_phase_4[] PROGMEM = "Full Moon";
static const char moon_phase_5[] PROGMEM = "Waning Gibbous";
static const char moon_phase_6[] PROGMEM = "Last Quarter";
static const char moon_phase_7[] PROGMEM = "Waning Crescent";

static const char* const MoonPhaseNames[] PROGMEM = {
    moon_phase_0, moon_phase_1, moon_phase_2, moon_phase_3,
    moon_phase_4, moon_phase_5, moon_phase_6, moon_phase_7
};

// Moon phase descriptions, minimum size is 128 bytes to fit longest string
static const char moon_desc_0[] PROGMEM = "The Moon is between Earth and Sun.\r\nThe illuminated side faces away from\r\nus, so it's effectively invisible.";
static const char moon_desc_1[] PROGMEM = "A sliver of the moon becomes visible\r\nas it starts to wax.";
static const char moon_desc_2[] PROGMEM = "Half of the moon is illuminated as\r\n it continues to wax.";
static const char moon_desc_3[] PROGMEM = "More than half of the moon is\r\nilluminated, approaching full moon.";
static const char moon_desc_4[] PROGMEM = "The entire face of the moon is\r\nilluminated.";
static const char moon_desc_5[] PROGMEM = "More than half of the moon is\r\nilluminated, starting to wane.";
static const char moon_desc_6[] PROGMEM = "Half of the moon is illuminated as\r\nit continues to wane.";
static const char moon_desc_7[] PROGMEM = "A sliver of the moon remains\r\nvisible as it nears new moon.";

static const char* const MoonPhaseDescriptions[] PROGMEM = {
    moon_desc_0, moon_desc_1, moon_desc_2, moon_desc_3,
    moon_desc_4, moon_desc_5, moon_desc_6, moon_desc_7
};

// Moon phase sea descriptions, minimum size is 64 bytes to fit longest string
static const char moon_sea_0[] PROGMEM = "Darker nights, minimal\r\nmoonlight.";
static const char moon_sea_1[] PROGMEM = "Increasing night visibility\r\nafter sunset.";
static const char moon_sea_2[] PROGMEM = "Strong visual reference in\r\nthe evening sky.";
static const char moon_sea_3[] PROGMEM = "Bright nights, increasing\r\ntidal range.";
static const char moon_sea_4[] PROGMEM = "Brightest nights, strongest\r\nspring tides.";
static const char moon_sea_5[] PROGMEM = "Bright late-night and\r\nearly-morning light.";
static const char moon_sea_6[] PROGMEM = "Rises around midnight,\r\nvisible in early morning.";
static const char moon_sea_7[] PROGMEM = "Low pre-dawn light, calming\r\nvisual indicator of cycle reset.";

static const char* const MoonPhaseSeaDescriptions[] PROGMEM = {
    moon_sea_0, moon_sea_1, moon_sea_2, moon_sea_3,
    moon_sea_4, moon_sea_5, moon_sea_6, moon_sea_7
};

// average length of lunar month in days
static constexpr double SYNODIC_MONTH = 29.53058867;

class Astronomy
{
public:
    /**
     * @brief Return moon age in days (0 = new moon, ~29.53 = next new moon).
     * @param unixTimestamp Seconds since epoch (UTC)
     * @return Age in days (floating)
     */
    static float getMoonAgeDays(unsigned long unixTimestamp) {
        // Convert Unix time to Julian Day (JD)
        // JD for unix epoch: 2440587.5
        double jd = unixTimestamp / 86400.0 + 2440587.5;
        // Reference new moon near Jan 6, 2000 -> JD 2451550.1 (commonly used reference)
        double daysSinceRef = jd - 2451550.1;
        double fraction = fmod(daysSinceRef / SYNODIC_MONTH, 1.0);
        if (fraction < 0.0) fraction += 1.0;
        double age = fraction * SYNODIC_MONTH;
        return (float)age;
    }

    /**
     * @brief Calculate approximate MoonPhase from unix timestamp.
     * Uses a simple astronomical approximation based on Julian date and average synodic month.
     * @param unixTimestamp Seconds since epoch (UTC)
     * @return MoonPhase enum value
     */
    static MoonPhase getMoonPhaseFromUnix(unsigned long unixTimestamp) {
        float age = getMoonAgeDays(unixTimestamp);
        // Map age into 8 equal sectors
        double phaseFraction = age / SYNODIC_MONTH; // 0..1
        int index = (int)(phaseFraction * 8.0 + 0.5) % 8;
        switch (index) {
        case 0: return MoonPhase::NewMoon;
        case 1: return MoonPhase::WaxingCrescent;
        case 2: return MoonPhase::FirstQuarter;
        case 3: return MoonPhase::WaxingGibbous;
        case 4: return MoonPhase::FullMoon;
        case 5: return MoonPhase::WaningGibbous;
        case 6: return MoonPhase::LastQuarter;
        default: return MoonPhase::WaningCrescent;
        }
    }

    // Copy the name from PROGMEM into a RAM buffer (safe copy)
    static size_t getMoonPhaseName(MoonPhase phase, char* buffer, size_t bufferSize) {
        if (!buffer || bufferSize == 0) return 0;
        uint8_t idx = static_cast<uint8_t>(phase);
        PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&MoonPhaseNames[idx]));
        strncpy_P(buffer, p, bufferSize);
        buffer[bufferSize - 1] = '\0';
        return strlen(buffer);
    }

    static size_t getMoonPhaseDescription(MoonPhase phase, char* buffer, size_t bufferSize) {
        if (!buffer || bufferSize == 0) return 0;
        uint8_t idx = static_cast<uint8_t>(phase);
        PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&MoonPhaseDescriptions[idx]));
        strncpy_P(buffer, p, bufferSize);
        buffer[bufferSize - 1] = '\0';
        return strlen(buffer);
    }

    static size_t getMoonPhaseSeaDescription(MoonPhase phase, char* buffer, size_t bufferSize) {
        if (!buffer || bufferSize == 0) return 0;
        uint8_t idx = static_cast<uint8_t>(phase);
        PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&MoonPhaseSeaDescriptions[idx]));
        strncpy_P(buffer, p, bufferSize);
        buffer[bufferSize - 1] = '\0';
        return strlen(buffer);
    }
};
