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

/**
 * @enum WarningType
 * @brief Extensible warning types for system monitoring using bit flags.
 *
 * Add new warning types as needed by extending this enum.
 * The manager tracks which warnings are currently active using a bitmap.
 *
 * Bit ranges:
 * - 0-19: System warnings
 * - 20-31: Sensor warnings
 */
enum class WarningType : uint32_t
{
    None = 0x00,                                      // No warning

    // System warnings (bits 0-19)
    DefaultConfigurationFuseBox = 1UL << 0,           // 0x00000001 - Using default config
    DefaultConfigurationControlPanel = 1UL << 1,      // 0x00000002 - Using default config
    ConnectionLost = 1UL << 2,                        // 0x00000004 - Connection lost warning
    HighCompassTemperature = 1UL << 3,                // 0x00000008 - Compass temperature threshold exceeded
    LowBattery = 1UL << 4,                            // 0x00000010 - Battery voltage low
	BluetoothInitFailed = 1UL << 5,                   // 0x00000020 - Bluetooth initialization failed
	WifiInitFailed = 1UL << 6,                        // 0x00000040 - WiFi initialization failed
	WifiInvalidConfig = 1UL << 7,                     // 0x00000080 - WiFi configuration invalid
	WeakWifiSignal = 1UL << 8,                        // 0x00000100 - WiFi signal weak
	SyncFailed = 1UL << 9,                            // 0x00000200 - Configuration sync issue detected
	SpiPinConfigError = 1UL << 10,                    // 0x00000400 - SD card configuration error
	SdCardError = 1UL << 11,                          // 0x00000800 - SD card read/write error
	SdCardMissing = 1UL << 12,                        // 0x00001000 - SD card not detected
	SdCardLowSpace = 1UL << 13,                       // 0x00002000 - SD card free space below 10%
	NextionInvalidConfig = 1UL << 14,                 // 0x00004000 - Nextion display configuration invalid

    // Sensor warnings (bits 20+)
    SensorFailure = 1UL << 20,                        // 0x00100000 - Sensor communication failure
    TemperatureSensorFailure = 1UL << 21,             // 0x00200000 - Temperature sensor failure
    CompassFailure = 1UL << 22,                       // 0x00400000 - Compass failed to initialize
	GpsFailure = 1UL << 23,                           // 0x00800000 - GPS sensor failure
};

// Warning type display strings - keep in sync with WarningType enum
// Index corresponds to bit position (0-31)
static const char WT_0[]  PROGMEM = "Fuse box Default Configuration";
static const char WT_1[]  PROGMEM = "Control Panel Default Configuration";
static const char WT_2[]  PROGMEM = "Connection Lost To Fuse Box";
static const char WT_3[]  PROGMEM = "High Compass Temperature";
static const char WT_4[]  PROGMEM = "Low Battery";
static const char WT_5[]  PROGMEM = "Bluetooth Init Failed";
static const char WT_6[]  PROGMEM = "Wifi Init Failed";
static const char WT_7[]  PROGMEM = "Wifi Invalid Config";
static const char WT_8[]  PROGMEM = "Weak Wifi Signal";
static const char WT_9[]  PROGMEM = "Synchronization Failed";
static const char WT_10[] PROGMEM = "SPI Pin Config Error";
static const char WT_11[] PROGMEM = "SD Card Error";
static const char WT_12[] PROGMEM = "SD Card Not Found";
static const char WT_13[] PROGMEM = "SD Card Low Space";
static const char WT_14[] PROGMEM = "Nextion Invalid Config";
static const char WT_15[] PROGMEM = "";
static const char WT_16[] PROGMEM = "";
static const char WT_17[] PROGMEM = "";
static const char WT_18[] PROGMEM = "";
static const char WT_19[] PROGMEM = "";
static const char WT_20[] PROGMEM = "Sensor Failure";
static const char WT_21[] PROGMEM = "Temperature Sensor Failure";
static const char WT_22[] PROGMEM = "Compass Failure";
static const char WT_23[] PROGMEM = "GPS Module Failure";
static const char WT_24[] PROGMEM = "";
static const char WT_25[] PROGMEM = "";
static const char WT_26[] PROGMEM = "";
static const char WT_27[] PROGMEM = "";
static const char WT_28[] PROGMEM = "";
static const char WT_29[] PROGMEM = "";
static const char WT_30[] PROGMEM = "";
static const char WT_31[] PROGMEM = "";

// Array of pointers (in PROGMEM) to the strings above.
// Note: pointers themselves are stored in flash; to read them use pgm_read_ptr.
static const char* const WARNING_TYPE_STRINGS_PROGMEM[32] PROGMEM = {
    WT_0,  WT_1,  WT_2,  WT_3,  WT_4,  WT_5,  WT_6,  WT_7,
    WT_8,  WT_9,  WT_10, WT_11, WT_12, WT_13, WT_14, WT_15,
    WT_16, WT_17, WT_18, WT_19, WT_20, WT_21, WT_22, WT_23,
    WT_24, WT_25, WT_26, WT_27, WT_28, WT_29, WT_30, WT_31
};
// Helper function to get warning string from bit position
inline const char* getWarningString(uint8_t bitPosition)
{
    static char buf[64];

    if (bitPosition < 32)
    {
        const char* progPtr = reinterpret_cast<const char*>(pgm_read_ptr(&WARNING_TYPE_STRINGS_PROGMEM[bitPosition]));

        if (progPtr == nullptr)
        {
            buf[0] = '\0';
            return buf;
        }

        // copy from PROGMEM into RAM buffer
        strncpy_P(buf, progPtr, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }

    buf[0] = '\0';
    return buf;
}

// Helper function to get warning string from WarningType
inline const char* getWarningString(WarningType type)
{
    if (type == WarningType::None)
    {
        return "No Warning";
    }

    uint32_t typeValue = static_cast<uint32_t>(type);

    // Find which bit is set (should only be one for valid WarningType)
    for (uint8_t bit = 0; bit < 32; bit++)
    {
        if (typeValue & (1UL << bit))
        {
            return getWarningString(bit);
        }
    }

    return "Unknown Warning";
}

// Bitwise operators for WarningType flags
inline WarningType operator|(WarningType a, WarningType b)
{
    return static_cast<WarningType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WarningType operator&(WarningType a, WarningType b)
{
    return static_cast<WarningType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline WarningType& operator|=(WarningType& a, WarningType b)
{
    return a = a | b;
}
