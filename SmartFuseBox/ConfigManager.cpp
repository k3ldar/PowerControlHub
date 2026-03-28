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
SystemHeader ConfigManager::_hdr;

void ConfigManager::begin()
{
    // On ESP platforms, EEPROM needs begin(size)
#if defined(ESP8266) || defined(ESP32)
	size_t requiredBytes = sizeof(SystemHeader) + sizeof(Config);
    EEPROM.begin(requiredBytes);
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
    EEPROM.get(sizeof(SystemHeader), _cfg);

    // Step up through every known version in sequence.
    // Each migration bumps _cfg.version by one, so a device that has missed
    // multiple releases will chain through all intermediate steps automatically.
    bool migrated = false;
    while (_cfg.version < ConfigVersion)
    {
        if (_cfg.version == ConfigVersion1)
        {
            migrateV1toV2();
            migrated = true;
        }
        else if (_cfg.version == ConfigVersion2)
        {
            migrateV2toV3();
            migrated = true;
		}
        else
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

void ConfigManager::migrateV1toV2()
{
    // V1 -> V2: SchedulerSettings was appended immediately before checksum.
    // Every V1 field occupies the same byte position in V2, so no data moves are needed.
    // The first two bytes of scheduler overlap where the V1 checksum was written;
    // the remainder of scheduler extends into EEPROM that V1 never touched.
    // Zero the entire scheduler section so all events start disabled and the
    // reserved bytes are clean before the new checksum is calculated by save().
    memset(&_cfg.scheduler, 0x00, sizeof(_cfg.scheduler));
    _cfg.version = ConfigVersion2;
}

void ConfigManager::migrateV2toV3()
{
    // V2 -> V3: SensorsConfig block appended before the top-level reserved bytes.
    // All existing V2 fields occupy the same byte positions, so no data moves are needed.
    // Zero the new sensors section so all entries start disabled and pins are cleared
    // before the new checksum is calculated by save().
    
    memset(&_cfg.sensors, 0x00, sizeof(_cfg.sensors));

    for (uint8_t i = 0; i < ConfigMaxSensors; ++i)
    {
        memset(_cfg.sensors.sensors[i].pins, 0xFF, ConfigMaxSensorPins);
    }

    _cfg.version = ConfigVersion;
}

bool ConfigManager::save()
{
    // prepare checksum
    _cfg.version = ConfigVersion;
    _cfg.checksum = 0;
    _cfg.checksum = calcChecksum(_cfg);

    EEPROM.put(sizeof(SystemHeader), _cfg);
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
    strncpy_P(_cfg.vessel.name, DefaultBoatName, ConfigMaxNameLength - 1);
    _cfg.vessel.name[ConfigMaxNameLength - 1] = '\0';  // Ensure null termination

    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        char shortBuf[ConfigShortRelayNameLength]{ 0 };
        snprintf_P(shortBuf, sizeof(shortBuf), RelayNameShort, (unsigned)i);
        strncpy(_cfg.relay.relays[i].shortName, shortBuf, ConfigShortRelayNameLength - 1);
        _cfg.relay.relays[i].shortName[ConfigShortRelayNameLength - 1] = '\0';

        char longBuf[ConfigLongRelayNameLength]{ 0 };
        snprintf_P(longBuf, sizeof(longBuf), RelayNameLong, (unsigned)i);
        strncpy(_cfg.relay.relays[i].longName, longBuf, ConfigLongRelayNameLength - 1);
        _cfg.relay.relays[i].longName[ConfigLongRelayNameLength - 1] = '\0';

        _cfg.relay.relays[i].buttonImage = 0x02;    // default color blue
        _cfg.relay.relays[i].defaultState = false;   // default off (relay closed)
#if defined(FUSE_BOX_CONTROLLER)
        _cfg.relay.relays[i].pin = Relays[i]; // default pin from Local.h
#else
        _cfg.relay.relays[i].pin = 0x255; // default value for no pin
#endif
    }

    // Reset linked relay table
    for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
    {
        _cfg.relay.linkedRelays[i][0] = 0xFF;
        _cfg.relay.linkedRelays[i][1] = 0xFF;
    }

    // Default home page mapping: first four relays visible in order
    for (uint8_t i = 0; i < ConfigHomeButtons; ++i)
    {
        _cfg.relay.homePageMapping[i] = i; // map slot i -> relay i
    }

	_cfg.vessel.vesselType = VesselType::Motor;
	_cfg.sound.hornRelayIndex = 0xFF; // none
    _cfg.lightSensor.nightRelayIndex = 0xFF; // none
    _cfg.lightSensor.daytimeThreshold = 512;
    _cfg.sound.startDelayMs = 500; // 500ms


	strncpy(_cfg.vessel.mmsi, "000000000", ConfigMmsiLength - 1);
	strncpy(_cfg.vessel.callSign, "NOCALL", ConfigCallSignLength - 1);
	strncpy(_cfg.vessel.homePort, "Unknown", ConfigHomePortLength - 1);
	_cfg.system.timezoneOffset = 0; // UTC

    _cfg.network.bluetoothEnabled = false;

	_cfg.network.wifiEnabled = false;
	_cfg.network.accessMode = WifiMode::AccessPoint;
	strncpy(_cfg.network.ssid, "SmartFuseBox", sizeof(_cfg.network.ssid) - 1);
	_cfg.network.ssid[sizeof(_cfg.network.ssid) - 1] = '\0';
	SystemFunctions::GenerateDefaultPassword(_cfg.network.password, sizeof(_cfg.network.password));
	_cfg.network.password[sizeof(_cfg.network.password) - 1] = '\0';
	_cfg.network.port = DefaultWifiPort;
	strncpy(_cfg.network.apIpAddress, DefaultApIpAddress, sizeof(_cfg.network.apIpAddress) - 1);
	_cfg.network.apIpAddress[sizeof(_cfg.network.apIpAddress) - 1] = '\0';

#if defined(WIFI_SUPPORT)
    _cfg.network.wifiEnabled = true;
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

    _cfg.sdCard.initializeSpeed = 4;
    _cfg.led.dayBrightness = 80;
    _cfg.led.dayGoodColor[0] = 0;
    _cfg.led.dayGoodColor[1] = 80;
    _cfg.led.dayGoodColor[2] = 255;
    _cfg.led.dayBadColor[0] = 255;
    _cfg.led.dayBadColor[1] = 140;
    _cfg.led.dayBadColor[2] = 0;

    _cfg.led.nightBrightness = 20;
    _cfg.led.nightGoodColor[0] = 100;
    _cfg.led.nightGoodColor[1] = 0;
    _cfg.led.nightGoodColor[2] = 0;
    _cfg.led.nightBadColor[0] = 255;
    _cfg.led.nightBadColor[1] = 50;
    _cfg.led.nightBadColor[2] = 0;

    _cfg.led.autoSwitch = true;
    _cfg.led.gpsEnabled = true;
    _cfg.led.warningEnabled = true;
    _cfg.led.systemEnabled = true;

    // default sound config
    _cfg.sound.goodPreset     = static_cast<uint8_t>(TonePreset::SubmarinePing);
    _cfg.sound.goodToneHz    = 1000;
    _cfg.sound.goodDurationMs = 100;
    _cfg.sound.badPreset      = static_cast<uint8_t>(TonePreset::DescendingAlert);
    _cfg.sound.badToneHz     = 500;
    _cfg.sound.badDurationMs = 200;
	_cfg.sound.badRepeatMs = 60000;

    for (uint8_t i = 0; i < ConfigMaxSensors; ++i)
        memset(_cfg.sensors.sensors[i].pins, 0xFF, ConfigMaxSensorPins);

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

bool ConfigManager::loadHeader()
{
    EEPROM.get(0, _hdr);

    if (_hdr.magic != SystemHeaderMagic || calcHeaderChecksum(_hdr) != _hdr.checksum)
    {
        memset(&_hdr, 0x00, sizeof(_hdr));
        _hdr.magic = SystemHeaderMagic;
        _hdr.headerVersion = 1;
        saveHeader();
    }

    return true;
}

bool ConfigManager::saveHeader()
{
    _hdr.checksum = 0;
    _hdr.checksum = calcHeaderChecksum(_hdr);
    EEPROM.put(0, _hdr);
#if defined(ESP8266) || defined(ESP32)
    return EEPROM.commit();
#else
    return true;
#endif
}

void ConfigManager::resetCrashCounter()
{
    _hdr.crashCounter = 0;
    _hdr.bootCount++;
    saveHeader();
}

SystemHeader* ConfigManager::getHeaderPtr()
{
    return &_hdr;
}

uint16_t ConfigManager::calcHeaderChecksum(const SystemHeader& h)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&h);
    size_t bytes = sizeof(SystemHeader) - sizeof(h.checksum);
    uint32_t sum = 0;
    for (size_t i = 0; i < bytes; ++i) sum += p[i];
    return static_cast<uint16_t>(sum & 0xFFFF);
}