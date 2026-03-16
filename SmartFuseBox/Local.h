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
  * 2) Set ConfigRelayCount and the Relays array to match your hardware.
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


// ─── Controller Mode ──────────────────────────────────────────────────────────
// Undefine to use only the underlying components (WiFi, MQTT etc.) without
// the fuse box relay controller logic.
#define FUSE_BOX_CONTROLLER


// ─── Optional Features ────────────────────────────────────────────────────────
// Remove the trailing underscore to enable each feature.

// SD card support (experimental)
#define SD_CARD_SUPPORT_

#if defined(SD_CARD_SUPPORT)
// Read config.txt from SD card at boot. Requires SD_CARD_SUPPORT.
#define CARD_CONFIG_LOADER_
#endif

// MQTT home-assistant discovery (requires WIFI_SUPPORT — enforced in BoardConfig.h)
#define MQTT_SUPPORT_

// Bluetooth BLE (mutually exclusive with WIFI_SUPPORT on Arduino Uno R4)
#define BLUETOOTH_SUPPORT_

// Led Manager on arduino R4 Wifi, experimental at present time
#if defined(ARDUINO_UNO_R4)
#define LED_MANAGER_
#endif

// ─── Serial ───────────────────────────────────────────────────────────────────
// Maximum milliseconds to wait for serial connection before proceeding with setup.
constexpr unsigned long serialInitTimeoutMs = 300;


// ─── Sensor Pins ──────────────────────────────────────────────────────────────
#if defined(ESP32)
// ESP32 NodeMCU-32S — adjust to your actual sensor wiring.
// ADC1 pins are safe; ADC2 pins (0,2,4,12-15,25-27) conflict with WiFi when active.
constexpr uint8_t WaterSensorPin = 34;       // ADC1, input-only, safe for analog read
constexpr uint8_t WaterSensorActivePin = 21; // GPIO21, output-capable, used to drive sensor power/enable
constexpr uint8_t Dht11SensorPin = 4;
constexpr uint8_t LightSensorPin = 36;       // ADC1 (VP), input-only, safe
constexpr bool LightSensorIsDigital = false;
#else
constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t WaterSensorActivePin = 8;
constexpr uint8_t Dht11SensorPin = 9;
// LightSensorPin can be used for either a digital or analog light sensor. Indicate 
// whether digital (on/off) sensor, of false for analog (light level) sensor
constexpr uint8_t LightSensorPin = 3;
constexpr bool LightSensorIsDigital = true;
#endif

#if defined(SD_CARD_SUPPORT)
#if defined(ESP32)
constexpr uint8_t SdCardCsPin = 5;
constexpr uint8_t SdCardMosiPin = 23;
constexpr uint8_t SdCardMisoPin = 19;
constexpr uint8_t SdCardSckPin = 18;
#else
constexpr uint8_t SdCardCsPin = 10;
constexpr uint8_t SdCardMosiPin = 11;
constexpr uint8_t SdCardMisoPin = 12;
constexpr uint8_t SdCardSckPin = 13;
#endif
#endif


// ─── Relay Config ─────────────────────────────────────────────────────────────
// ConfigRelayCount must match the number of entries in Relays[].
constexpr uint8_t ConfigRelayCount = 8;

#if defined(ESP32)
// ESP32 NodeMCU-32S safe GPIO pins (avoids flash SPI pins 6-11 and strapping pins).
// Adjust these to match your actual relay board wiring.
constexpr uint8_t Relay1 = 32;
constexpr uint8_t Relay2 = 33;
constexpr uint8_t Relay3 = 25;
constexpr uint8_t Relay4 = 26;
constexpr uint8_t Relay5 = 27;
constexpr uint8_t Relay6 = 14;
constexpr uint8_t Relay7 = 12;
constexpr uint8_t Relay8 = 13;
#else
constexpr uint8_t Relay1 = 7;
constexpr uint8_t Relay2 = 6;
constexpr uint8_t Relay3 = 5;
constexpr uint8_t Relay4 = 4;
constexpr uint8_t Relay5 = 19;
constexpr uint8_t Relay6 = 18;
constexpr uint8_t Relay7 = 17;
constexpr uint8_t Relay8 = 16;
#endif

constexpr uint8_t Relays[ConfigRelayCount] = { Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8 };


// ─── Framework Config ─────────────────────────────────────────────────────────
// BoardConfig.h reads the defines above and derives EEPROM capacity,
// WIFI_SUPPORT, SCHEDULER_SUPPORT, and validates combinations.
// Do not edit BoardConfig.h — it is updated by the upstream repository.
#include "BoardConfig.h"


// ─── Network Config ───────────────────────────────────────────────────────────
constexpr uint8_t MaxConcurrentClients = 2;
constexpr uint8_t MaxPersistentClients = 1;