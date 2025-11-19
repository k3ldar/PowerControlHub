#pragma once 

#include <Arduino.h>
#include <stdint.h>

enum class WarningType : uint32_t {
    None = 0x00,                                // No warning

    // System warnings (bits 0-19)
    DefaultConfiguration = 1ULL << 0,           // 0x01 - Using default config
    ConnectionLost = 1ULL << 1,                 // 0x02 - Link heartbeat lost
    LowBattery = 1ULL << 3,                     // 0x08 - Battery voltage low

    // Sensor warnings (bits 20+)
    SensorFailure = 1ULL << 20,                 // Sensor communication failure
    TemperatureSensorFailure = 1ULL << 21,      // Temperature sensor failure
    CompassFailure = 1ULL << 22,                 // 0x10 - Compass failed to initialize
    HighCompassTemperature = 1ULL << 23,        // 0x80 - Compass temperature threshold exceeded (sensor warning)
};


constexpr int DefaultDelay = 5;
constexpr unsigned long serialInitTimeoutMs = 300;


constexpr uint8_t WaterSensorPin = A0;
constexpr uint8_t WaterSensorActivePin = D8;
constexpr uint8_t Dht11SensorPin = D9;


// Digital pins for relays
constexpr uint8_t Relay4 = D4;
constexpr uint8_t Relay3 = D5;
constexpr uint8_t Relay2 = D6;
constexpr uint8_t Relay1 = D7;

// Analog pins used as digital (A2–A5 → 16–19)
constexpr uint8_t Relay8 = 16;
constexpr uint8_t Relay7 = 17;
constexpr uint8_t Relay6 = 18;
constexpr uint8_t Relay5 = 19;

constexpr uint8_t TotalRelays = 8;

constexpr uint8_t Relays[TotalRelays] = { Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8 };


constexpr char SystemHeartbeatCommand[] = "F0";
constexpr char SystemInitializedCommand[] = "F1";
constexpr char SystemFreeMemoryCommand[] = "F2";

constexpr char SensorWaterLevel[] = "S6";
constexpr char SensorTemperature[] = "S0";
constexpr char SensorHumidity[] = "S1";

constexpr char WarningAdd[] = "W4";


constexpr char AckSuccess[] = "ok";
constexpr char ValueParamName[] = "v";


constexpr unsigned long SerialInitTimeoutMs = 300;

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
