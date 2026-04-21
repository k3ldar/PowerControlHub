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

#include "SystemDefinitions.h"
#include "Local.h"


constexpr uint8_t ConfigMaxScheduledEvents = MAXIMUM_EVENTS;
constexpr uint8_t ConfigSchedulerPayloadSize = 4;
constexpr uint8_t ConfigScheduleEventReserved = 8;
constexpr uint8_t ConfigSchedulerReservedSize = 10;

// MQTT Configuration
constexpr uint8_t ConfigMqttBrokerLength = 64;
constexpr uint8_t ConfigMqttUsernameLength = 32;
constexpr uint8_t ConfigMqttPasswordLength = 32;
constexpr uint8_t ConfigMqttDeviceIdLength = 32;
constexpr uint8_t ConfigMqttDiscoveryPrefixLength = 32;
constexpr uint16_t ConfigMqttDefaultPort = 1883;
constexpr uint16_t ConfigMqttKeepAliveDefault = 60;

// sensors
constexpr uint8_t ConfigMaxSensors = 8;
constexpr uint8_t ConfigMaxSensorNameLength = 21;
constexpr uint8_t ConfigMaxSensorPins = 4;

constexpr uint32_t SystemHeaderMagic = 0x53464201;  // 'SFB\x01'

// Flags stored in SystemHeader::reserved[0]
constexpr uint8_t OtaFlagAutoApply = 0x01;  // bit0: automatically download and apply updates

constexpr uint8_t ConfigAuthApiKeyLength = 32;
constexpr uint8_t ConfigAuthHmacKeyLength = 32;

struct SystemHeader {
    uint32_t magic;               // offset 0  — must equal SystemHeaderMagic
    uint32_t bootCount;           // offset 4
    uint16_t configWrittenBy;     // offset 8
    uint8_t  headerVersion;       // offset 10 — for future header migrations
    uint8_t  crashCounter;        // offset 11
    uint8_t  lastResetReason;     // offset 12
    uint8_t  safeModeFlagsf;      // offset 13
    uint8_t  reserved[16];        // offset 14 — reserved[0] = OTA flags (see OtaFlagAutoApply)
    uint16_t checksum;            // offset 30 — always last
} __attribute__((packed));        // = 32 bytes

enum class LocationType : uint8_t
{
    Power = 0x00,                   // Power boat
    Sail = 0x01,                    // Sailing boat
    Fishing = 0x02,                 // Fishing boat
    Yacht = 0x03,                   // Yacht
	Shed = 0x04,                    // Shed
	Basement = 0x05,                // Basement
	Workshop = 0x06,                // Workshop
	Garage = 0x07,                  // Garage
	Bedroom = 0x08,                 // Bedroom
	Office = 0x09,                  // Office
	Other = 0xFF                    // Other/unspecified
};

enum class TriggerType : uint8_t
{
	None      = 0x00,
	TimeOfDay = 0x01,  // triggerPayload[0]=hour, [1]=minute
	Sunrise   = 0x02,  // triggerPayload[0..1]=int16 offset minutes before/after
	Sunset    = 0x03,  // triggerPayload[0..1]=int16 offset minutes before/after
	Interval  = 0x04,  // triggerPayload[0..1]=uint16 interval minutes
	DayOfWeek = 0x05,  // triggerPayload[0]=bitmask (bit0=Mon .. bit6=Sun)
	Date      = 0x06,  // triggerPayload[0]=day, [1]=month
};

enum class ConditionType : uint8_t
{
	None       = 0x00,
	TimeWindow = 0x01,  // conditionPayload[0]=start_hour, [1]=start_min, [2]=end_hour, [3]=end_min
	DayOfWeek  = 0x02,  // conditionPayload[0]=bitmask (bit0=Mon .. bit6=Sun)
	IsDark     = 0x03,  // no payload (computed from sunrise/sunset)
	IsDaylight = 0x04,  // no payload (computed from sunrise/sunset)
	RelayIsOn  = 0x05,  // conditionPayload[0]=relay index
	RelayIsOff = 0x06,  // conditionPayload[0]=relay index
};

enum class ExecutionActionType : uint8_t
{
	None         = 0x00,
	RelayOn      = 0x01,  // actionPayload[0]=relay index
	RelayOff     = 0x02,  // actionPayload[0]=relay index
	RelayToggle  = 0x03,  // actionPayload[0]=relay index
	RelayPulse   = 0x04,  // actionPayload[0]=relay index, [1..2]=uint16 duration seconds
	AllRelaysOn  = 0x05,
	AllRelaysOff = 0x06,
	SetPinHigh = 0x07,  // actionPayload[0]=pin number
	SetPinLow = 0x08,  // actionPayload[0]=pin number
};

enum class RelayActionType : uint8_t
{
	Default    = 0x00,
	Horn       = 0x01,  // Replaces SoundConfig::hornRelayIndex
	NightRelay = 0x02,  // Replaces LightSensorConfig::nightRelayIndex
};


// Keep struct packed and stable. Increase 'VERSION' when you change layout.
// Packed POD for persistent configuration.
constexpr uint8_t ConfigVersion1 = 1;
constexpr uint8_t ConfigVersion2 = 2;
constexpr uint8_t ConfigVersion3 = 3;
constexpr uint8_t ConfigVersion4 = 4;
constexpr uint8_t ConfigVersion5 = 5;
constexpr uint8_t ConfigVersion6 = 6;
constexpr uint8_t ConfigVersion = ConfigVersion6;

constexpr uint8_t ConfigHomeButtons = 4;
constexpr uint8_t ConfigMaxNameLength = 31; // max characters (inc null)
constexpr uint8_t ConfigShortRelayNameLength = 6; // max characters (inc null) - for home page
constexpr uint8_t ConfigLongRelayNameLength = 21; // max characters (inc null) - for buttons page
constexpr uint8_t ConfigMmsiLength = 10; // 9 chars + null
constexpr uint8_t ConfigHomePortLength = 31; // 30 chars + null
constexpr uint8_t ConfigCallSignLength = 10; // 9 chars + null
constexpr uint8_t ConfigLinkedRelayCount = 2;

struct SystemConfig {
    int8_t  timezoneOffset;
    uint8_t reserved1[4];
    int8_t  reserved2[4];
} __attribute__((packed));

struct LocationConfig {
    char name[ConfigMaxNameLength];
    LocationType locationType;
    char mmsi[ConfigMmsiLength];
    char callSign[ConfigCallSignLength];
    char homePort[ConfigHomePortLength];
    uint8_t reserved1[4];
    int8_t reserved2[4];
} __attribute__((packed));

struct RelayEntry {
    char shortName[ConfigShortRelayNameLength];
    char longName[ConfigLongRelayNameLength];
    uint8_t pin;
    uint8_t buttonImage;
    bool defaultState;
    RelayActionType actionType; 
    uint8_t reserved[1];
} __attribute__((packed));

struct RelayConfig {
    uint8_t homePageMapping[ConfigHomeButtons]; 
    RelayEntry relays[ConfigRelayCount];
    uint8_t linkedRelays[ConfigMaxLinkedRelays][ConfigLinkedRelayCount];
    uint8_t reserved1[4];
    int8_t reserved2[4];
} __attribute__((packed));

struct NetworkConfig {
    bool wifiEnabled;
    WifiMode accessMode;
    char ssid[MaxSSIDLength];
    char password[MaxWiFiPasswordLength];
    uint16_t port;
    char apIpAddress[MaxIpAddressLength];
    bool bluetoothEnabled; 
    uint8_t reserved1[4];
    int8_t reserved2[4];
} __attribute__((packed));

struct SoundConfig {
    uint8_t hornRelayIndex;
    uint16_t startDelayMs;
    uint8_t  goodPreset;
    uint16_t goodToneHz;
    uint16_t goodDurationMs;
    uint8_t  badPreset;
    uint16_t badToneHz;
    uint16_t badDurationMs;
    uint32_t badRepeatMs;
    uint8_t reserved1[2];
    int8_t reserved2[2];
} __attribute__((packed));

struct SdCardConfig {
    uint8_t initializeSpeed;
    uint8_t csPin;
    uint8_t reserved[3];
} __attribute__((packed));

struct LedConfig {
    // Day Mode
    uint8_t dayBrightness;
    uint8_t dayGoodColor[3];
    uint8_t dayBadColor[3];

    // Night Mode
    uint8_t nightBrightness;
    uint8_t nightGoodColor[3];
    uint8_t nightBadColor[3];

    // Auto-switching
    bool autoSwitch;            // Enable/disable auto day/night

    // Per-LED enable
    bool gpsEnabled;
    bool warningEnabled;
    bool systemEnabled;
} __attribute__((packed));

struct LightSensorConfig
{
    uint8_t nightRelayIndex;    // 0..7 or 0xFF = none
    uint16_t daytimeThreshold;  // 0..1023 ADC threshold
} __attribute__((packed));

struct MqttConfig
{
    bool enabled;
    char broker[ConfigMqttBrokerLength];
    uint16_t port;
    char username[ConfigMqttUsernameLength];
    char password[ConfigMqttPasswordLength];
    char deviceId[ConfigMqttDeviceIdLength];
    bool useHomeAssistantDiscovery;
    uint16_t keepAliveInterval;
    char discoveryPrefix[ConfigMqttDiscoveryPrefixLength];
} __attribute__((packed));

struct ScheduledEvent
{
    bool enabled;
    TriggerType triggerType;
    uint8_t triggerPayload[ConfigSchedulerPayloadSize];
    ConditionType conditionType;
    uint8_t conditionPayload[ConfigSchedulerPayloadSize];
    ExecutionActionType actionType;
    uint8_t actionPayload[ConfigSchedulerPayloadSize];
    int8_t reserved[ConfigScheduleEventReserved];
} __attribute__((packed));

struct SchedulerSettings
{
    bool isEnabled;
    uint8_t eventCount;
    ScheduledEvent events[ConfigMaxScheduledEvents];
    uint8_t reserved[ConfigSchedulerReservedSize];
} __attribute__((packed));

struct SensorEntry {
    bool enabled;
    SensorIdList sensorType;
    uint8_t pins[ConfigMaxSensorPins];
    char name[ConfigMaxSensorNameLength];
    int8_t options1[2];
    int16_t options2[2];
} __attribute__((packed));

struct SensorsConfig {
    uint8_t count;
    SensorEntry sensors[ConfigMaxSensors];
    uint8_t reserved1[2];
    int8_t reserved2[2];
} __attribute__((packed));

// XpdzTone: buzzer/piezo tone alert configuration
struct XpdzTone {
    uint8_t pin;
    uint8_t reserved[4];
} __attribute__((packed));

// Hw479Rgb: RGB LED warning indicator pin configuration, PinDisabled = not fitted
struct Hw479Rgb {
    uint8_t rPin;
    uint8_t gPin;
    uint8_t bPin;
    uint8_t reserved1[3];
    int8_t  reserved2[2];
} __attribute__((packed));

// RtcConfig: DS1302 real-time clock pin configuration, PinDisabled = not fitted
struct RtcConfig {
    uint8_t dataPin;          // DS1302 DAT pin; 
    uint8_t clockPin;         // DS1302 CLK pin
    uint8_t resetPin;         // DS1302 RST/CE pin
    uint8_t reserved1[3];
    int8_t  reserved2[2];
} __attribute__((packed));

struct SpiPins {
    uint8_t sckPin;
    uint8_t misoPin;
    uint8_t mosiPin;
} __attribute__((packed));

struct NetworkAuthConfig {
    bool enabled;
    uint8_t version;
    char apiKey[ConfigAuthApiKeyLength];
    char hmacKey[ConfigAuthHmacKeyLength];
    uint8_t reserved[4];
} __attribute__((packed));

struct NextionConfig {
	bool enabled;
	bool isHardwareSerial;  // if false, use SoftwareSerial on rxPin/txPin
	uint8_t rxPin;
	uint8_t txPin;
	uint32_t baudRate;
	uint8_t uartNum;        // ESP32: UART peripheral index — valid values 1 or 2 only (UART0 is reserved for USB/debug)
	uint8_t reserved[4];
} __attribute__((packed));

struct RemoteSensorConfig
{
    SensorIdList sensorId;
	char name[21];   // friendly name for UI
    char mqttName[16];
    char mqttSlug[16];
    char mqttTypeSlug[16];
    char mqttDeviceClass[16];
    char mqttUnit[11];
    bool mqttIsBinary;
    uint8_t reserved[8];
} __attribute__((packed));


struct RemoteSensorsConfig {
    uint8_t count;
    RemoteSensorConfig sensors[ConfigMaxSensors];
    uint8_t reserved1[2];
    int8_t reserved2[2];
} __attribute__((packed));

struct Config {
    uint8_t version;
    SystemConfig system;
    LocationConfig location;
    RelayConfig relay;
    NetworkConfig network;
    SoundConfig sound;
    LedConfig led;
    MqttConfig mqtt;
    LightSensorConfig lightSensor;
    SdCardConfig sdCard;
    SchedulerSettings scheduler;
    SensorsConfig sensors;
    SpiPins spiPins;
    NetworkAuthConfig auth;
	XpdzTone xpdzTone;
	Hw479Rgb hw479Rgb;
    RtcConfig rtc;
    NextionConfig nextion;
    RemoteSensorsConfig remoteSensors;
    uint8_t reserved1[2];
    int8_t reserved2[2];
    uint16_t checksum;          // always last
} __attribute__((packed));

static_assert(sizeof(Config) + sizeof(SystemHeader) <= EEPROM_CAPACITY_BYTES, "Config struct exceeds EEPROM capacity for this board. Reduce features or increase EEPROM capacity.");
