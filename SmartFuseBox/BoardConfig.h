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

/*
 * BoardConfig.h - framework-owned board capability table (do not edit)
 *
 * This file is included by Local.h and reads the board selection and feature
 * opt-ins defined there to derive capability flags, EEPROM sizing, and
 * scheduler support. It also enforces invalid-combination guards.
 *
 * To add a new board: add an entry to the EEPROM table and the WIFI_SUPPORT
 * block below. Local.h does not need to change.
 */


// ─── Board Conflict Guards ────────────────────────────────────────────────────
#if defined(ARDUINO_UNO_R4)
  #if defined(ESP32) || defined(ARDUINO_R4_MINIMA)
    #error "Multiple board types defined. Activate exactly one in Local.h."
  #endif
#endif

#if defined(ESP32)
  #if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
    #error "Multiple board types defined. Activate exactly one in Local.h."
  #endif
#endif

#if defined(ARDUINO_R4_MINIMA)
  #if defined(ARDUINO_UNO_R4) || defined(ESP32)
    #error "Multiple board types defined. Activate exactly one in Local.h."
  #endif
#endif

#if !defined(ARDUINO_UNO_R4) && !defined(ESP32) && !defined(ARDUINO_R4_MINIMA)
  #error "No board type defined. Activate one in Local.h (e.g. ARDUINO_UNO_R4)."
#endif


// ─── EEPROM Capacity Table ────────────────────────────────────────────────────
// Add new board entries here. Local.h does not need to change.
#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
  #define EEPROM_CAPACITY_BYTES 8192
#elif defined(ESP32)
  #define EEPROM_CAPACITY_BYTES 16384
#else
  #define EEPROM_CAPACITY_BYTES 1024
  #define MAXIMUM_EVENTS 6
#endif

#if !defined(MAXIMUM_EVENTS)
  #define MAXIMUM_EVENTS 20
#endif


// ─── Derived: Scheduler Support ───────────────────────────────────────────────
#if EEPROM_CAPACITY_BYTES >= 1024
  #define SCHEDULER_SUPPORT
#endif


// ─── Derived: WiFi Support ────────────────────────────────────────────────────
#if defined(ARDUINO_UNO_R4) || defined(ESP32)
  #define WIFI_SUPPORT
#endif


// ─── Feature Combination Guards ───────────────────────────────────────────────
#if defined(MQTT_SUPPORT) && !defined(WIFI_SUPPORT)
  #error "MQTT_SUPPORT requires WIFI_SUPPORT. Enable a WiFi-capable board in Local.h."
#endif

#if defined(ARDUINO_UNO_R4) && defined(WIFI_SUPPORT) && defined(BLUETOOTH_SUPPORT)
  #error "WIFI_SUPPORT and BLUETOOTH_SUPPORT cannot both be active on Arduino Uno R4. Disable one in Local.h."
#endif
