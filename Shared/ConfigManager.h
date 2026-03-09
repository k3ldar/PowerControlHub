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
#pragma once
#include <Arduino.h>
#include "Config.h"

class ConfigManager
{
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