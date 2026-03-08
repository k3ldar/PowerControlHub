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

#define BOAT_CONTROL_PANEL
#define ARDUINO_MEGA2560
//#define ARDUINO_R4_MINIMA

// Uncomment the following line to enable the screen demo command handler, which cycles through 
// all pages with a specified interval (default 6 seconds), this is a blocking operation and 
// should only be used for testing/demo purposes.
//#define SCREEN_DEMO_SUPPORT


constexpr uint8_t ConfigRelayCount = 8;

/*
 * EEPROM capacity in bytes for the selected board.
 * Add new board entries above the #else fallback when adding new board support.
 */
#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
#define EEPROM_CAPACITY_BYTES 8192
#elif defined(ESP32)
#define EEPROM_CAPACITY_BYTES 16384
#elif defined(ARDUINO_MEGA2560)
#define EEPROM_CAPACITY_BYTES 4096
#else
 // maximum 6 events to save eeprom size
#define MAXIMUM_EVENTS 6
#define EEPROM_CAPACITY_BYTES 1024   
#endif

#if !defined(MAXIMUM_EVENTS)
//default events
#define MAXIMUM_EVENTS 20
#endif
