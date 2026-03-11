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


enum class VesselType : uint8_t
{
    Motor = 0x00,                   // Power boat
    Sail = 0x01,                    // Sailing boat
    Fishing = 0x02,                 // Fishing boat
    Yacht = 0x03,                   // Yacht
};

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

struct SoundSignalConfig
{
	uint8_t goodPreset;
	uint16_t good_toneHz;
	uint16_t good_durationMs;
	uint8_t badPreset;
	uint16_t bad_toneHz;
	uint16_t bad_durationMs;
	uint32_t bad_repeatMs;
} __attribute__((packed));


struct LightSensorConfig
{
	uint8_t nightRelayIndex;    // 0..7 or 0xFF = none
	uint16_t daytimeThreshold;  // 0..1023 ADC threshold
} __attribute__((packed));


// MQTT Configuration
constexpr uint8_t ConfigMqttBrokerLength = 64;
constexpr uint8_t ConfigMqttUsernameLength = 32;
constexpr uint8_t ConfigMqttPasswordLength = 32;
constexpr uint8_t ConfigMqttDeviceIdLength = 32;
constexpr uint8_t ConfigMqttDiscoveryPrefixLength = 32;
constexpr uint16_t ConfigMqttDefaultPort = 1883;
constexpr uint16_t ConfigMqttKeepAliveDefault = 60;

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


constexpr uint8_t AccessModeAP = 0;
constexpr uint8_t AccessModeClient = 1;

#if defined(SCHEDULER_SUPPORT)

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

enum class SchedulerActionType : uint8_t
{
	None         = 0x00,
	RelayOn      = 0x01,  // actionPayload[0]=relay index
	RelayOff     = 0x02,  // actionPayload[0]=relay index
	RelayToggle  = 0x03,  // actionPayload[0]=relay index
	RelayPulse   = 0x04,  // actionPayload[0]=relay index, [1..2]=uint16 duration seconds
	AllRelaysOn  = 0x05,
	AllRelaysOff = 0x06,
};

constexpr uint8_t ConfigMaxScheduledEvents = MAXIMUM_EVENTS;
constexpr uint8_t ConfigSchedulerPayloadSize = 4;
constexpr uint8_t ConfigScheduleEventReserved = 8;
constexpr uint8_t ConfigSchedulerReservedSize = 10;

struct ScheduledEvent
{
	bool enabled;
	TriggerType triggerType;
	uint8_t triggerPayload[ConfigSchedulerPayloadSize];
	ConditionType conditionType;
	uint8_t conditionPayload[ConfigSchedulerPayloadSize];
	SchedulerActionType actionType;
	uint8_t actionPayload[ConfigSchedulerPayloadSize];
	uint8_t reserved[ConfigScheduleEventReserved];
} __attribute__((packed));

struct SchedulerSettings
{
	bool isEnabled;
	uint8_t eventCount;
	ScheduledEvent events[ConfigMaxScheduledEvents];
	uint8_t reserved[ConfigSchedulerReservedSize];  // reserved for future scheduler-level settings
} __attribute__((packed));

#endif // SCHEDULER_SUPPORT


// Layout:
// - version (uint8_t)
// - boatName (30 chars + null)
// - relayShortNames[8][6] (5 chars + null each)
// - relayLongNames[8][21] (20 chars + null each)
// - homePageMapping[4] (values 0..7 or 0xFF for empty)
// - homePageButtonImage[4] (button color image IDs)
// - vesselType (VesselType)
// - hornRelayIndex (uint8_t) 0..7 or 0xFF = none
// - checksum (uint16_t)
//
// Keep struct packed and stable. Increase 'VERSION' when you change layout.
// Packed POD for persistent configuration.
#if defined(SCHEDULER_SUPPORT)
constexpr uint8_t ConfigVersion = 2;
#else
constexpr uint8_t ConfigVersion = 1;
#endif

constexpr uint8_t ConfigHomeButtons = 4;
constexpr uint8_t ConfigMaxNameLength = 31; // max characters (inc null)
constexpr uint8_t ConfigShortRelayNameLength = 6; // max characters (inc null) - for home page
constexpr uint8_t ConfigLongRelayNameLength = 21; // max characters (inc null) - for buttons page
constexpr uint8_t ConfigMmsiLength = 10; // 9 chars + null
constexpr uint8_t ConfigHomePortLength = 31; // 30 chars + null
constexpr uint8_t ConfigCallSignLength = 10; // 9 chars + null
constexpr uint8_t ConfigLinkededRelayCount = 2;

struct Config {
    uint8_t version;
    char name[ConfigMaxNameLength];
    char relayShortNames[ConfigRelayCount][ConfigShortRelayNameLength];
    char relayLongNames[ConfigRelayCount][ConfigLongRelayNameLength];
    uint8_t homePageMapping[ConfigHomeButtons]; // 0..7 or 0xFF = empty
    uint8_t buttonImage[ConfigRelayCount]; // 0..7 or 0xFF = empty
    VesselType vesselType;
	uint8_t hornRelayIndex; // 0..7 or 0xFF = none
	uint16_t soundStartDelayMs; // 500ms default
    bool defaulRelayState[ConfigRelayCount]; // default on (relay open) states
    uint8_t linkedRelays[ConfigMaxLinkedRelays][ConfigLinkededRelayCount];
	char mMSI[ConfigMmsiLength]; // 9 chars + null
	char callSign[ConfigCallSignLength]; // 9 chars + null
	char homePort[ConfigHomePortLength]; // 30 chars + null
	int8_t timezoneOffset; // hours from UTC

    bool bluetoothEnabled;

	bool wifiEnabled;
	uint8_t accessMode; // 0 = AP, 1 = Client
    char apSSID[MaxSSIDLength];
    char apPassword[MaxWiFiPasswordLength];
    uint16_t wifiPort;
	char apIpAddress[MaxIpAddressLength]; // xxx.xxx.xxx.xxx + null

	uint8_t sdCardInitializeSpeed;

	LedConfig ledConfig;
	SoundSignalConfig soundConfig;

	MqttConfig mqtt;

	LightSensorConfig lightSensor;

	#if defined(SCHEDULER_SUPPORT)
	SchedulerSettings scheduler;
#endif

	uint16_t checksum;
} __attribute__((packed));

static_assert(sizeof(Config) <= EEPROM_CAPACITY_BYTES, "Config struct exceeds EEPROM capacity for this board. Reduce features or increase EEPROM capacity.");
