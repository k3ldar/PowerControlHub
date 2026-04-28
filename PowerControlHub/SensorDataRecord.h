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
 * @enum SensorDataType
 * @brief Types of sensor data that can be logged
 */
enum class SensorDataType : uint8_t {
    Temperature = 0x01,
    Humidity = 0x02,
    WaterLevel = 0x03,
    LightLevel = 0x04,
};

/**
 * @struct SensorDataRecord
 * @brief Standardized structure for logging sensor data
 * 
 * This structure holds a single sensor reading with timestamp.
 * Designed to be compact for efficient memory usage in circular buffer.
 */
struct SensorDataRecord {
    uint64_t timestamp;        // SystemFunctions::millis64() when reading was taken
    SensorDataType sensorType;      // Type of sensor data
    float value1;                   // Primary value (e.g., temperature, water level)
    float value2;                   // Secondary value (e.g., humidity, average water level)
    
    SensorDataRecord() 
        : timestamp(0), sensorType(SensorDataType::Temperature), value1(0.0f), value2(0.0f)
    {
    }
    
    SensorDataRecord(uint64_t ts, SensorDataType type, float val1, float val2 = 0.0f)
        : timestamp(ts), sensorType(type), value1(val1), value2(val2)
    {
    }
};
