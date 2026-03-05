#include <Arduino.h>
#include <stdint.h>

#pragma once
/*
 * Local.h - per-device configuration (consumer editable)
 *
 * Purpose:
 * - This file is intended for consumers to configure pins, feature flags,
 *   and small board-specific constants for their local hardware.
 * - Make changes here for your device (pins, relay count, feature enables).
 * - Avoid committing any secrets or user-specific runtime changes back to
 *   the main repository. Treat changes in this file as local configuration.
 *
 * Rules / Checklist when editing:
 * 1) Update the feature defines (e.g. `FUSE_BOX_CONTROLLER`, `MQTT_SUPPORT`) as needed.
 * 2) Set `ConfigRelayCount` to the number of relays used on your board.
 *    Then update the `Relays` array to contain exactly that many pin constants.
 * 3) Keep the constant names (e.g. `WaterSensorPin`) stable; other code
 *    uses these identifiers when constructing the application and sensors.
 * 4) If you change pin numbers, verify any late-binding calls in `SmartFuseBox.ino`
 *    (for example setting `SdCardLogger`) still apply correctly.
 */


/*
 * Board selection. Exactly one of the defines below must be active (no trailing underscore).
 *
 * NAMING CONVENTION used throughout Local.h:
 *   A define WITH a trailing underscore (e.g. `ARDUINO_R4_MINIMA_`) is INACTIVE.
 *   Remove the trailing underscore to activate it (e.g. `ARDUINO_R4_MINIMA`).
 *   This lets you keep all options visible without deleting them.
 *
 * ARDUINO_UNO_R4		– Arduino Uno R4 WiFi (u-blox NINA / ESP32-S3 radio module, BLE + WiFi).
 * ARDUINO_R4_MINIMA	– Arduino Uno R4 Minima (no radio; disables WIFI_SUPPORT / BLUETOOTH_SUPPORT).
 * ESP32				– ESP32 NodeMCU-32 or compatible (WiFi + BLE via built-in radio).
 *
 * Sanity-check #error directives later in this file will catch multiple active selections.
 */
#define ARDUINO_UNO_R4
#define ARDUINO_R4_MINIMA_
#define ESP32_

/*
* Determines whether to include code related to the smart fuse box controller functionality. If you are using this code as a
* library for a different type of project, you can undefine this to exclude the controller-specific code and just use the 
* underlying components (e.g. WifiController, MQTTClient) in your own application.
*/
#define FUSE_BOX_CONTROLLER

/*
* Wifi is only available on certain boards (e.g. Arduino Uno R4). Define WIFI_SUPPORT to enable related code, but only if your board supports it.
*/
#if defined(ARDUINO_UNO_R4) || defined(ESP32)
#define WIFI_SUPPORT
#endif


/*
 * SD card Support. Remove the trailing underscore to enable.
 * When enabled the system will enabled SD Card support
 * (see SdCard* constants below). This feature is experimental and disabled by default.
 */
#define SD_CARD_SUPPORT

#if defined(SD_CARD_SUPPORT)
/*
 * SD card config loader. Remove the trailing underscore to enable.
 * Only supported if SD_CARD_SUPPORT define is configured
 * When enabled the system reads `config.txt` from the SD card at boot (and on card-insert)
 * so settings can be changed without reflashing. Requires SD card SPI pins wired correctly
 * (see SdCard* constants below). This feature is experimental and disabled by default.
 */
#define CARD_CONFIG_LOADER_
#endif


#if defined(WIFI_SUPPORT)

/*
* MQTT relies on WIFI_SUPPORT
* If using with home assistant MQTT discovery, define MQTT_SUPPORT to include the MQTTController and related handlers.
*/
#define MQTT_SUPPORT_

#endif

/*
 * Bluetooth BLE support. Remove the trailing underscore to enable.
 *
 * Only available on boards with a BLE radio (e.g. Arduino Uno R4 WiFi, ESP32).
 * NOTE: On Arduino Uno R4 WiFi, BLUETOOTH_SUPPORT and WIFI_SUPPORT are mutually exclusive —
 * the ANNA-B112 (BLE) and ESP32-S3 (WiFi) modules share the same RF path and cannot
 * run simultaneously. The #error guard below will catch this if both are active.
 */
#define BLUETOOTH_SUPPORT_

// Serial initialization timeout. If `waitForConnection` is true when initializing serial, the system will wait up 
// to this many milliseconds for the serial connection to be established before proceeding with setup. This can help 
// ensure that you see any log messages or errors that occur during startup, especially if your 
// board resets when you open the serial monitor.
constexpr unsigned long serialInitTimeoutMs = 300;

/*
* The following pins are project specific defaults for the example sensors included with the project. You can change 
* these pin numbers to match your hardware wiring. If you add new sensors, you can define additional pins here as 
* needed and update your sensor handlers to use them.
*/

// Sensor pins
constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t LightSensorAnalogPin = A1;
constexpr uint8_t LightSensorPin = D3;
constexpr uint8_t WaterSensorActivePin = D8;
constexpr uint8_t Dht11SensorPin = D9;

#if defined(SD_CARD_SUPPORT)
// SD card SPI pins (used by MicroSdDriver)
constexpr uint8_t SdCardCsPin = D10;
constexpr uint8_t SdCardMosiPin = D11;
constexpr uint8_t SdCardMisoPin = D12;
constexpr uint8_t SdCardSckPin = D13;
#endif

/* 
* Number of relays in the hardware. Keep this in sync with `Relays` array. You MUST have at least 1 relay 
* for the system to function, but you can set this to less than the number of defined relay pins if your 
* board or setup doesn't use them all.
*/
constexpr uint8_t ConfigRelayCount = 8;

// Digital pins for relays (consumer should update if their board differs)
constexpr uint8_t Relay4 = D4;
constexpr uint8_t Relay3 = D5;
constexpr uint8_t Relay2 = D6;
constexpr uint8_t Relay1 = D7;

/* Relay pins mapped to analog-header pins used in digital mode.
*  On Arduino Uno R4 / R4 Minima, A2–A5 are exposed as integer indices 16–19.
*  If you are using a different board, replace these with the correct digital pin numbers.
* */
constexpr uint8_t Relay8 = 16;
constexpr uint8_t Relay7 = 17;
constexpr uint8_t Relay6 = 18;
constexpr uint8_t Relay5 = 19;

// Relay pin mapping. Must contain exactly ConfigRelayCount entries.
// Array index 0 is relay 1, index 1 is relay 2, and so on — the command layer
// uses this order when addressing relays by number. Keep Relay1 first.
constexpr uint8_t Relays[ConfigRelayCount] = { Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8 };


#if defined(ARDUINO_UNO_R4) && defined(WIFI_SUPPORT) && defined(BLUETOOTH_SUPPORT)
#error "WIFI_SUPPORT and BLUETOOTH_SUPPORT cannot both be enabled on Arduino Uno R4. Disable one in Local.h."
#endif

#if defined(ARDUINO_UNO_R4) 
#if defined(ESP32) || defined(ARDUINO_R4_MINIMA)
#error "Multiple board types defined. Please only define the board you are using (e.g. ARDUINO_UNO_R4 or ESP32)."
#endif
#endif

#if defined(ESP32)
#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
#error "Multiple board types defined. Please only define the board you are using (e.g. ARDUINO_UNO_R4 or ESP32)."
#endif
#endif

#if defined(ARDUINO_R4_MINIMA)
#if defined(ARDUINO_UNO_R4) || defined(ESP32)
#error "Multiple board types defined. Please only define the board you are using (e.g. ARDUINO_UNO_R4 or ESP32)."
#endif
#endif

#if !defined(ARDUINO_UNO_R4) && !defined(ESP32) && !defined(ARDUINO_R4_MINIMA)
#error "No board type defined. Please define the board you are using (e.g. ARDUINO_UNO_R4 or ESP32)."
#endif