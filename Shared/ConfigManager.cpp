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
#include <EEPROM.h>

#include "Local.h"
#include "ConfigManager.h"
#include "SystemFunctions.h"

#include "ToneManager.h"

const char DefaultBoatName[] PROGMEM = "My Boat";
constexpr char RelayNameShort[] PROGMEM = "R %u";
constexpr char RelayNameLong[] PROGMEM = "Relay %u";
// Static storage (use the shared Config from Config.h)
Config ConfigManager::_cfg;

void ConfigManager::begin()
{
    // On ESP platforms, EEPROM needs begin(size)
#if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(sizeof(Config));
#endif
}

uint16_t ConfigManager::calcChecksum(const Config& c)
{
    // simple 16-bit sum over bytes excluding checksum field
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&c);
    size_t bytes = sizeof(Config) - sizeof(c.checksum);
    uint32_t sum = 0;
    for (size_t i = 0; i < bytes; ++i) sum += p[i];
    return static_cast<uint16_t>(sum & 0xFFFF);
}

bool ConfigManager::load()
{
    // Ensure EEPROM is initialised for boards that require it
    begin();

    // Read raw bytes
    EEPROM.get(0, _cfg);

    // Step up through every known version in sequence.
    // Each migration bumps _cfg.version by one, so a device that has missed
    // multiple releases will chain through all intermediate steps automatically.
    bool migrated = false;
    while (_cfg.version < ConfigVersion)
    {
#if defined(SCHEDULER_SUPPORT)
        if (_cfg.version == 1)
        {
            migrateV1toV2();
            migrated = true;
        }
        else
#endif
        {
            // Version is below the oldest known migration (e.g. 0x00 on some blank boards).
            // Cannot migrate safely — reset and persist clean defaults.
            resetToDefaults();
            save();
            return false;
        }
    }

    // Version is above ConfigVersion (future firmware downgraded, or 0xFF on a fresh board)
    if (_cfg.version != ConfigVersion)
    {
        resetToDefaults();
        save();
        return false;
    }

    // Migration succeeded — persist the upgraded config and skip the checksum check
    // (the old checksum is no longer valid for the new layout).
    if (migrated)
    {
        save();
        return true;
    }

    uint16_t expected = calcChecksum(_cfg);
    if (expected != _cfg.checksum)
    {
        // corrupted: reset to defaults and persist
        resetToDefaults();
        save();
        return false;
    }

    return true;
}

#if defined(SCHEDULER_SUPPORT)
void ConfigManager::migrateV1toV2()
{
    // V1 -> V2: SchedulerSettings was appended immediately before checksum.
    // Every V1 field occupies the same byte position in V2, so no data moves are needed.
    // The first two bytes of scheduler overlap where the V1 checksum was written;
    // the remainder of scheduler extends into EEPROM that V1 never touched.
    // Zero the entire scheduler section so all events start disabled and the
    // reserved bytes are clean before the new checksum is calculated by save().
    memset(&_cfg.scheduler, 0x00, sizeof(_cfg.scheduler));
    _cfg.version = ConfigVersion;
}
#endif // SCHEDULER_SUPPORT

bool ConfigManager::save()
{
    // prepare checksum
    _cfg.version = ConfigVersion;
    _cfg.checksum = 0;
    _cfg.checksum = calcChecksum(_cfg);

    EEPROM.put(0, _cfg);
#if defined(ESP8266) || defined(ESP32)
    // commit for ESP
    bool ok = EEPROM.commit();
    return ok;
#else
    // AVR writes immediately; assume OK
    return true;
#endif
}

void ConfigManager::resetToDefaults()
{
    // Erase to safe defaults
    memset(&_cfg, 0x00, sizeof(_cfg));
    _cfg.version = ConfigVersion;

    // Default boat name
    strncpy_P(_cfg.name, DefaultBoatName, ConfigMaxNameLength - 1);
    _cfg.name[ConfigMaxNameLength - 1] = '\0';  // Ensure null termination

    // Default relay names (both short and long)
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        // Default short name: "R0", "R1", etc.
        char shortBuf[ConfigShortRelayNameLength]{ 0 };
        snprintf_P(shortBuf, sizeof(shortBuf), RelayNameShort, (unsigned)i);
        strncpy(_cfg.relayShortNames[i], shortBuf, ConfigShortRelayNameLength - 1);
        _cfg.relayShortNames[i][ConfigShortRelayNameLength - 1] = '\0';

        // Default long name: "Relay 0", "Relay 1", etc.
        char longBuf[ConfigLongRelayNameLength]{ 0 };
        snprintf_P(longBuf, sizeof(longBuf), RelayNameLong, (unsigned)i);
        strncpy(_cfg.relayLongNames[i], longBuf, ConfigLongRelayNameLength - 1);
        _cfg.relayLongNames[i][ConfigLongRelayNameLength - 1] = '\0';
    }

    // Default home page mapping: first four relays visible in order
    for (uint8_t i = 0; i < ConfigHomeButtons; ++i)
    {
        _cfg.homePageMapping[i] = i; // map slot i -> relay i
    }

    // Default home page mapping: first four relays visible in order
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        _cfg.buttonImage[i] = 0x02; // default color blue
    }

	_cfg.vesselType = VesselType::Motor;
	_cfg.hornRelayIndex = 0xFF; // none
    _cfg.lightSensor.nightRelayIndex = 0xFF; // none
    _cfg.lightSensor.daytimeThreshold = 512;
    _cfg.soundStartDelayMs = 500; // 500ms
	
    // default relay states
	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        _cfg.defaulRelayState[i] = false; // default off (relay closed)
    }

    // reset linked relays
    for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
    {
        _cfg.linkedRelays[i][0] = 0xFF;
        _cfg.linkedRelays[i][1] = 0xFF;
    }

	strncpy(_cfg.mMSI, "000000000", ConfigMmsiLength - 1);
	strncpy(_cfg.callSign, "NOCALL", ConfigCallSignLength - 1);
	strncpy(_cfg.homePort, "Unknown", ConfigHomePortLength - 1);
	_cfg.timezoneOffset = 0; // UTC

    _cfg.bluetoothEnabled = false;

	_cfg.wifiEnabled = true;
	_cfg.accessMode = AccessModeAP; // 0 = AP, 1 = Client
	strncpy(_cfg.apSSID, "SmartFuseBox", sizeof(_cfg.apSSID) - 1);
	_cfg.apSSID[sizeof(_cfg.apSSID) - 1] = '\0';
	SystemFunctions::GenerateDefaultPassword(_cfg.apPassword, sizeof(_cfg.apPassword));
	_cfg.apPassword[sizeof(_cfg.apPassword) - 1] = '\0';
	_cfg.wifiPort = DefaultWifiPort;
	strncpy(_cfg.apIpAddress, DefaultApIpAddress, sizeof(_cfg.apIpAddress) - 1);
	_cfg.apIpAddress[sizeof(_cfg.apIpAddress) - 1] = '\0';

#if defined(WIFI_SUPPORT)
    _cfg.wifiEnabled = false;
#endif

#if defined(MQTT_SUPPORT)
	// MQTT defaults
	_cfg.mqtt.enabled = false;
	strncpy(_cfg.mqtt.broker, "192.168.1.100", ConfigMqttBrokerLength - 1);
	_cfg.mqtt.broker[ConfigMqttBrokerLength - 1] = '\0';
	_cfg.mqtt.port = ConfigMqttDefaultPort;
	_cfg.mqtt.username[0] = '\0';
	_cfg.mqtt.password[0] = '\0';
	strncpy(_cfg.mqtt.deviceId, "smartfusebox_01", ConfigMqttDeviceIdLength - 1);
	_cfg.mqtt.deviceId[ConfigMqttDeviceIdLength - 1] = '\0';
	_cfg.mqtt.useHomeAssistantDiscovery = false;
	_cfg.mqtt.keepAliveInterval = ConfigMqttKeepAliveDefault;
	strncpy(_cfg.mqtt.discoveryPrefix, "homeassistant", ConfigMqttDiscoveryPrefixLength - 1);
	_cfg.mqtt.discoveryPrefix[ConfigMqttDiscoveryPrefixLength - 1] = '\0';
#else
    _cfg.mqtt.enabled = false;
#endif

    _cfg.sdCardInitializeSpeed = 4;
    _cfg.ledConfig.dayBrightness = 80;
    _cfg.ledConfig.dayGoodColor[0] = 0;
    _cfg.ledConfig.dayGoodColor[1] = 80; 
    _cfg.ledConfig.dayGoodColor[2] = 255;
    _cfg.ledConfig.dayBadColor[0] = 255;
    _cfg.ledConfig.dayBadColor[1] = 140;
    _cfg.ledConfig.dayBadColor[2] = 0;

    _cfg.ledConfig.nightBrightness = 20;
    _cfg.ledConfig.nightGoodColor[0] = 100;
    _cfg.ledConfig.nightGoodColor[1] = 0;
    _cfg.ledConfig.nightGoodColor[2] = 0;
    _cfg.ledConfig.nightBadColor[0] = 255;
    _cfg.ledConfig.nightBadColor[1] = 50;
    _cfg.ledConfig.nightBadColor[2] = 0;

    _cfg.ledConfig.autoSwitch = true;
    _cfg.ledConfig.gpsEnabled = true;
    _cfg.ledConfig.warningEnabled = true;
    _cfg.ledConfig.systemEnabled = true;

    // default sound config
    _cfg.soundConfig.goodPreset     = static_cast<uint8_t>(TonePreset::SubmarinePing);
    _cfg.soundConfig.good_toneHz    = 1000;
    _cfg.soundConfig.good_durationMs = 100;
    _cfg.soundConfig.badPreset      = static_cast<uint8_t>(TonePreset::DescendingAlert);
    _cfg.soundConfig.bad_toneHz     = 500;
    _cfg.soundConfig.bad_durationMs = 200;
	_cfg.soundConfig.bad_repeatMs = 60000;

    // compute checksum
    _cfg.checksum = 0;
    _cfg.checksum = calcChecksum(_cfg);
}

Config* ConfigManager::getConfigPtr()
{
    return &_cfg;
}

size_t ConfigManager::availableEEPROMBytes()
{
    return EEPROM.length();
}