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
#include <stdint.h>

 /*
  * Local.h - per-device configuration (consumer editable)
  *
  * This file is the ONLY file you need to edit for your hardware.
  * All derived flags, board capability tables, and validation guards
  * live in BoardConfig.h which is included at the bottom of this file.
  * BoardConfig.h is framework-owned — do not edit it.
  *
  * Rules:
  * 1) Activate exactly ONE board define below.
  * 2) Set ConfigRelayCount to the number of relay channels on your hardware.
  * 3) Opt in/out of features using the trailing-underscore convention.
  * 4) Do not commit hardware-specific changes back to the main repository.
  */


  // ─── Board Selection ──────────────────────────────────────────────────────────
  // Activate exactly ONE. Remove the trailing underscore to enable.
  // ARDUINO_UNO_R4   – Arduino Uno R4 WiFi (BLE + WiFi)
  // ARDUINO_R4_MINIMA – Arduino Uno R4 Minima (no radio)
  // ESP32_           – ESP32 NodeMCU-32 or compatible (WiFi + BLE) 
#define ARDUINO_UNO_R4_
#define ARDUINO_R4_MINIMA_

#if !defined(ESP32) && !defined(ARDUINO_UNO_R4) && !defined(ARDUINO_R4_MINIMA)
#define ESP32
#endif


// CONFIGURE_SPI gates the board-specific SPI.begin(sck, miso, mosi) overload used
// by cores that support custom pin assignment at bus-init time (e.g. ESP32).
// When undefined, the portable SPI.begin() is called with the core's default pins.
// Pin values passed to beginInitialize() are always stored regardless of this flag,
// so board-specific code can still apply them (e.g. via SPI.setSCK/setMISO/setMOSI)
// if the target core provides those helpers.
#define CONFIGURE_SPI
#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
#undef CONFIGURE_SPI
#endif


// ─── Display Support ───────────────────────────────────────────────────────
// Enable support for Nextion displays (approx 52kb).
#define NEXTION_DISPLAY_DEVICE

// ─── Optional Features ────────────────────────────────────────────────────────
// Remove the trailing underscore to enable each feature.

// SD card support (experimental) (approx 32kb)
#define SD_CARD_SUPPORT

#if defined(SD_CARD_SUPPORT)
// Read config.txt from SD card at boot. Requires SD_CARD_SUPPORT.
#define CARD_CONFIG_LOADER_
#endif

// MQTT home-assistant discovery (requires WIFI_SUPPORT — enforced in BoardConfig.h) (approx 40kb)
#define MQTT_SUPPORT

// OTA auto-update via GitHub releases (requires ESP32 + WIFI_SUPPORT — enforced in BoardConfig.h).
// When enabled, the device checks for a new release every 24 hours and broadcasts the result.
// Auto-applying the update is OFF by default; use the F12:apply=1 command or set the
// OtaFlagAutoApply bit in SystemHeader::reserved[0] to enable automatic installs. (approx 165kb)
#define OTA_AUTO_UPDATE

// Bluetooth BLE (mutually exclusive with WIFI_SUPPORT on Arduino Uno R4) (approx 144kb)
#define BLUETOOTH_SUPPORT_

// Led Manager on arduino R4 Wifi, experimental at present time
#if defined(ARDUINO_UNO_R4)
#define LED_MANAGER_
#endif

// ─── Serial ───────────────────────────────────────────────────────────────────
// Maximum milliseconds to wait for serial connection before proceeding with setup.
constexpr uint64_t serialInitTimeoutMs = 300;


// ─── Relay Config ─────────────────────────────────────────────────────────────
// Number of relay channels. Pins are assigned at runtime via configuration.
constexpr uint8_t ConfigRelayCount = 8;


// ─── Framework Config ─────────────────────────────────────────────────────────
// BoardConfig.h reads the defines above and derives EEPROM capacity,
// WIFI_SUPPORT and validates combinations.
// Do not edit BoardConfig.h — it is updated by the upstream repository.
#include "BoardConfig.h"


// ─── Network Config ───────────────────────────────────────────────────────────
constexpr uint8_t MaxConcurrentClients = 8;
constexpr uint8_t MaxPersistentClients = 1;