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
    unsigned long timestamp;        // millis() when reading was taken
    SensorDataType sensorType;      // Type of sensor data
    float value1;                   // Primary value (e.g., temperature, water level)
    float value2;                   // Secondary value (e.g., humidity, average water level)
    
    SensorDataRecord() 
        : timestamp(0), sensorType(SensorDataType::Temperature), value1(0.0f), value2(0.0f) {}
    
    SensorDataRecord(unsigned long ts, SensorDataType type, float val1, float val2 = 0.0f)
        : timestamp(ts), sensorType(type), value1(val1), value2(val2) {}
};
