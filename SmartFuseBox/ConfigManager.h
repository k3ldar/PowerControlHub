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

constexpr uint8_t CrashCounterThreshold = 3;

class ConfigManager
{
private:
	static uint16_t calcChecksum(const Config& c);
	static bool loadHeader();
	static bool saveHeader();
	static uint16_t calcHeaderChecksum(const SystemHeader& h);
	static void migrateV1toV2();
	static void migrateV2toV3();
	static void migrateV3toV4();
	static void migrateV4toV5();
	static void migrateV5toV6();
	static Config _cfg;
	static SystemHeader _hdr;

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

    // Call after full system initialisation succeeds to reset the crash counter
    static void resetCrashCounter();

    // Access current in-memory system header
    static SystemHeader* getHeaderPtr();
};