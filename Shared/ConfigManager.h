#pragma once
#include <Arduino.h>
#include "Config.h"

class ConfigManager {
public:
    // Call once at startup (handles any board-specific init)
    static void begin();

    // Load config from EEPROM. Returns true if valid config loaded.
    static bool load();

    // Save current config to EEPROM. Returns true on success.
    static bool save();

    // Reset config in memory back to sane defaults (does not write to EEPROM).
    static void resetToDefaults();

    // Access current in-memory config
    static Config* getConfigPtr();

    // For debug or UI: how many bytes of EEPROM are available
    static size_t availableEEPROMBytes();

private:
    static uint16_t calcChecksum(const Config& c);
#if defined(SCHEDULER_SUPPORT)
    static void migrateV1toV2();
#endif
    static Config _cfg;
};