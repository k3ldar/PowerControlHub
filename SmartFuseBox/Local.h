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
* Type of board you are using. This is used to conditionally compile features based on the board capabilities. For example, 
* if you are using an Arduino Uno R4 or ESP32 that supports wifi, you can define ARDUINO_UNO_R4 to enable Bluetooth and WiFi support.
*/
#define ARDUINO_UNO_R4
#define ARDUINO_R4_MINIMA_
#define ESP32_NODE_32_

/*
* Determines whether to include code related to the smart fuse box controller functionality. If you are using this code as a
* library for a different type of project, you can undefine this to exclude the controller-specific code and just use the 
* underlying components (e.g. WifiController, MQTTClient) in your own application.
*/
#define FUSE_BOX_CONTROLLER

/*
* Wifi is only available on certain boards (e.g. Arduino Uno R4). Define WIFI_SUPPORT to enable related code, but only if your board supports it.
*/
#define WIFI_SUPPORT

/*
* If you have an SD Card reader you can load the configuration from the Sd card at startup, or when a card is inserted. 
* Please note this is not working correctly just yet, so it's disabled by default. If you enable it, 
* the system will look for a file named `config.txt` on the SD card and attempt to load configuration from it. 
* This allows you to update settings without needing to reprogram the device, which can be useful for users who
* are not comfortable with flashing firmware. If you enable this, make sure to also connect the SD card reader to the correct 
* SPI pins defined in Local.h.
*/
#define CARD_CONFIG_LOADER_

#if defined(WIFI_SUPPORT)

/*
* If using with home assistant MQTT discovery, define MQTT_SUPPORT to include the MQTTController and related handlers.
*/
#define MQTT_SUPPORT

#endif

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

// SD card SPI pins (used by MicroSdDriver)
constexpr uint8_t SdCardCsPin = D10;
constexpr uint8_t SdCardMosiPin = D11;
constexpr uint8_t SdCardMisoPin = D12;
constexpr uint8_t SdCardSckPin = D13;

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

// Analog pins used as digital (A2–A5 → 16–19)
constexpr uint8_t Relay8 = 16;
constexpr uint8_t Relay7 = 17;
constexpr uint8_t Relay6 = 18;
constexpr uint8_t Relay5 = 19;

// Relay pin mapping. Must contain exactly ConfigRelayCount entries.
constexpr uint8_t Relays[ConfigRelayCount] = { Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8 };


/*
* Sanity check only one board selected
*/
#if defined(ESP32_NODE_32) && !defined(ESP32)
#define ESP32
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