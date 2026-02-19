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

#if defined(MQQT_SUPPORT)
// MQTT Configuration
constexpr uint8_t ConfigMqttBrokerLength = 64;
constexpr uint8_t ConfigMqttUsernameLength = 32;
constexpr uint8_t ConfigMqttPasswordLength = 32;
constexpr uint8_t ConfigMqttDeviceIdLength = 32;
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
} __attribute__((packed));
#endif 

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
constexpr uint8_t ConfigVersion = 1;
constexpr uint8_t ConfigRelayCount = 8;
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

#if defined(ARDUINO_UNO_R4)
    bool bluetoothEnabled;
	bool wifiEnabled;
	uint8_t accessMode; // 0 = AP, 1 = Client
    char apSSID[MaxSSIDLength];
    char apPassword[MaxWiFiPasswordLength];
    uint16_t wifiPort;
	char apIpAddress[MaxIpAddressLength]; // xxx.xxx.xxx.xxx + null
#endif

	uint8_t sdCardInitializeSpeed;

	LedConfig ledConfig;
	SoundSignalConfig soundConfig;

#if defined(MQQT_SUPPORT)
	MqttConfig mqtt;
#endif

	uint16_t checksum;
} __attribute__((packed));