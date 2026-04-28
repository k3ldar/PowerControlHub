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

#include <stdint.h>

constexpr uint16_t DefaultSoundStartDelayMs = 500;

// COLREGS Sound Signal Durations
constexpr uint16_t SoundBlastShortMs = 1000;  // ~1 second (COLREGS Rule 34)
constexpr uint16_t SoundBlastLongMs = 5000;   // 4-6 seconds (COLREGS Rule 34)
constexpr uint16_t SoundBlastGapMs = 1500;    // Gap between blasts in COLREGS sequences

// SOS-specific durations (Morse code timing for electronic signal)
constexpr uint16_t MorseCodeShortMs = 500;         // Dot duration
constexpr uint16_t MorseCodeLongMs = 1000;          // Dash duration (3x dot)
constexpr uint16_t MorseCodeGapMs = 400;            // Gap between dots/dashes

// COLREGS Repeat Intervals
constexpr uint32_t FogRepeatMs = 120000;  // 2 minutes (COLREGS Rule 35)
constexpr uint32_t SosRepeatMs = 10000;   // 10 seconds (distress signal)
constexpr uint32_t NoRepeat = 0;           // One-shot signals
