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
#define ARDUINO_UNO_R4
#define ARDUINO_R4_MINIMA_
#define ESP32_


// ─── Controller Mode ──────────────────────────────────────────────────────────
// Undefine to use only the underlying components (WiFi, MQTT etc.) without
// the fuse box relay controller logic.
#define FUSE_BOX_CONTROLLER


// ─── Optional Features ────────────────────────────────────────────────────────
// Remove the trailing underscore to enable each feature.

// SD card support (experimental)
#define SD_CARD_SUPPORT

#if defined(SD_CARD_SUPPORT)
// Read config.txt from SD card at boot. Requires SD_CARD_SUPPORT.
#define CARD_CONFIG_LOADER_
#endif

// MQTT home-assistant discovery (requires WIFI_SUPPORT — enforced in BoardConfig.h)
#define MQTT_SUPPORT_

// Bluetooth BLE (mutually exclusive with WIFI_SUPPORT on Arduino Uno R4)
#define BLUETOOTH_SUPPORT_


// ─── Serial ───────────────────────────────────────────────────────────────────
// Maximum milliseconds to wait for serial connection before proceeding with setup.
constexpr unsigned long serialInitTimeoutMs = 300;


// ─── Sensor Pins ──────────────────────────────────────────────────────────────
constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t LightSensorAnalogPin = A1;
constexpr uint8_t LightSensorPin = D3;
constexpr uint8_t WaterSensorActivePin = D8;
constexpr uint8_t Dht11SensorPin = D9;

#if defined(SD_CARD_SUPPORT)
constexpr uint8_t SdCardCsPin = D10;
constexpr uint8_t SdCardMosiPin = D11;
constexpr uint8_t SdCardMisoPin = D12;
constexpr uint8_t SdCardSckPin = D13;
#endif


// ─── Relay Config ─────────────────────────────────────────────────────────────
// ConfigRelayCount must match the number of entries in Relays[].
constexpr uint8_t ConfigRelayCount = 8;

constexpr uint8_t Relay1 = D7;
constexpr uint8_t Relay2 = D6;
constexpr uint8_t Relay3 = D5;
constexpr uint8_t Relay4 = D4;
constexpr uint8_t Relay5 = 19;
constexpr uint8_t Relay6 = 18;
constexpr uint8_t Relay7 = 17;
constexpr uint8_t Relay8 = 16;

constexpr uint8_t Relays[ConfigRelayCount] = { Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8 };


// ─── Framework Config ─────────────────────────────────────────────────────────
// BoardConfig.h reads the defines above and derives EEPROM capacity,
// WIFI_SUPPORT, SCHEDULER_SUPPORT, and validates combinations.
// Do not edit BoardConfig.h — it is updated by the upstream repository.
#include "BoardConfig.h"