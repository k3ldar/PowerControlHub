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


// ─── Sub Controller Mode ──────────────────────────────────────────────────────
// local to this repository, as local implementations of the same SmartFuseBox
#define SUB_CONTROLLER_WEATHER_STATION_
#define SUB_CONTROLLER_BASEMENT_SENSOR

#if defined(SUB_CONTROLLER_WEATHER_STATION) && defined(SUB_CONTROLLER_BASEMENT_SENSOR)
#error There can only be on sub controller mode active at a time. Please choose between WEATHER_STATION and MQTT_SENSOR.
#elif defined(SUB_CONTROLLER_WEATHER_STATION)
#define ARDUINO_UNO_R4
#elif defined(SUB_CONTROLLER_BASEMENT_SENSOR) && !defined(ESP32)
#define ESP32
#endif


// ─── Optional Features ────────────────────────────────────────────────────────
// Remove the trailing underscore to enable each feature.

// SD card support (experimental)
#define SD_CARD_SUPPORT_

#if defined(SD_CARD_SUPPORT)
// Read config.txt from SD card at boot. Requires SD_CARD_SUPPORT.
#define CARD_CONFIG_LOADER_
#endif

// MQTT home-assistant discovery (requires WIFI_SUPPORT — enforced in BoardConfig.h)
#define MQTT_SUPPORT

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
#if defined(SUB_CONTROLLER_WEATHER_STATION)

constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t WaterSensorActivePin = D8;
constexpr uint8_t Dht11SensorPin = D7;

constexpr bool LightSensorIsDigital = true;
constexpr uint8_t LightSensorPin = D2;

#elif defined(SUB_CONTROLLER_BASEMENT_SENSOR)

constexpr uint8_t Dht11StoreRoomSensorPin = 4;
constexpr uint8_t Dht11MakerRoomSensorPin = 5;
constexpr uint8_t Dht11PassagewaySensorPin = 25;
constexpr uint8_t Dht11WashRoomSensorPin = 26;

#else
#error Please select sub type
#endif

#if defined(SD_CARD_SUPPORT)
#if defined(ESP32)
constexpr uint8_t SdCardCsPin = 6;
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

#if defined(SUB_CONTROLLER_WEATHER_STATION)

constexpr uint8_t ConfigRelayCount = 4;

constexpr uint8_t Relay1 = D13;
constexpr uint8_t Relay2 = D12;
constexpr uint8_t Relay3 = D11;
constexpr uint8_t Relay4 = D10;

constexpr uint8_t Relays[ConfigRelayCount] = { Relay1, Relay2, Relay3, Relay4 };

#elif defined(ESP32)

constexpr uint8_t ConfigRelayCount = 1;

constexpr uint8_t Relay1 = 32;

constexpr uint8_t Relays[ConfigRelayCount] = { Relay1 };

#endif

// ─── Framework Config ─────────────────────────────────────────────────────────
// BoardConfig.h reads the defines above and derives EEPROM capacity,
// WIFI_SUPPORT and validates combinations.
// Do not edit BoardConfig.h — it is updated by the upstream repository.
#include "BoardConfig.h"


// ─── Network Config ───────────────────────────────────────────────────────────
#if defined(WIFI_SUPPORT)
constexpr uint8_t MaxConcurrentClients = 5;
constexpr uint8_t MaxPersistentClients = 1;
#endif