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

	loadHeader();

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
		else if (_cfg.version == ConfigVersion3)
		{
			migrateV3toV4();
			migrated = true;
		}
		else if (_cfg.version == ConfigVersion4)
		{
			migrateV4toV5();
			migrated = true;
		}
		else if (_cfg.version == ConfigVersion5)
		{
			migrateV5toV6();
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
		memset(_cfg.sensors.sensors[i].pins, PinDisabled, ConfigMaxSensorPins);
	}

	_cfg.version = ConfigVersion3;
}

void ConfigManager::migrateV3toV4()
{
	// V3 -> V4: RelayActionType field added to RelayEntry (consumes 1 reserved byte).
	// Promote hornRelayIndex -> Horn actionType, nightRelayIndex -> NightRelay actionType.
	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		_cfg.relay.relays[i].actionType = RelayActionType::Default;
	}

	if (_cfg.sound.hornRelayIndex < ConfigRelayCount)
	{
		_cfg.relay.relays[_cfg.sound.hornRelayIndex].actionType = RelayActionType::Horn;
	}

	if (_cfg.lightSensor.nightRelayIndex < ConfigRelayCount)
	{
		_cfg.relay.relays[_cfg.lightSensor.nightRelayIndex].actionType = RelayActionType::NightRelay;
	}

	_cfg.version = ConfigVersion4;
}

void ConfigManager::migrateV4toV5()
{
	// V4 -> V5: introduced user configurable SPI pins
	_cfg.spiPins.misoPin = PinDisabled;
	_cfg.spiPins.mosiPin = PinDisabled;
	_cfg.spiPins.sckPin = PinDisabled;
	_cfg.auth.enabled = false;
	_cfg.auth.version = 0;
	_cfg.auth.apiKey[0] = '\0';
	_cfg.auth.hmacKey[0] = '\0';
	memset(_cfg.auth.reserved, 0x00, sizeof(_cfg.auth.reserved));

	_cfg.version = ConfigVersion5;
	_cfg.system.rebootOnSave = false;
	_cfg.sdCard.csPin = PinDisabled;
}

void ConfigManager::migrateV5toV6()
{
	// V5 -> V6: XpdzTone, Hw479Rgb and RtcConfig added; all pins default to PinDisabled
	_cfg.xpdzTone.pin = PinDisabled;

	_cfg.hw479Rgb.rPin = PinDisabled;
	_cfg.hw479Rgb.gPin = PinDisabled;
	_cfg.hw479Rgb.bPin = PinDisabled;
	memset(_cfg.hw479Rgb.reserved1, 0x00, sizeof(_cfg.hw479Rgb.reserved1));
	memset(_cfg.hw479Rgb.reserved2, 0x00, sizeof(_cfg.hw479Rgb.reserved2));

	_cfg.rtc.dataPin = PinDisabled;
	_cfg.rtc.clockPin = PinDisabled;
	_cfg.rtc.resetPin = PinDisabled;
	memset(_cfg.rtc.reserved1, 0x00, sizeof(_cfg.rtc.reserved1));
	memset(_cfg.rtc.reserved2, 0x00, sizeof(_cfg.rtc.reserved2));

	_cfg.nextion.enabled = false;
	_cfg.nextion.isHardwareSerial = true;
	_cfg.nextion.baudRate = 19200;
	_cfg.nextion.uartNum = 2;
	_cfg.nextion.rxPin = PinDisabled;
	_cfg.nextion.txPin = PinDisabled;

	_cfg.remoteSensors.count = 0;
	_cfg.remoteSensors.reserved1[0] = 0x00;
	_cfg.remoteSensors.reserved1[1] = 0x00;
	_cfg.remoteSensors.reserved2[0] = 0x00;
	_cfg.remoteSensors.reserved2[1] = 0x00;

	for (uint8_t i = 0; i < ConfigMaxSensors; ++i)
	{
		_cfg.remoteSensors.sensors[i].sensorId = SensorIdList::None;
		// Clear fixed-size string buffers
		memset(_cfg.remoteSensors.sensors[i].name, 0x00, sizeof(_cfg.remoteSensors.sensors[i].name));
		memset(_cfg.remoteSensors.sensors[i].mqttName, 0x00, sizeof(_cfg.remoteSensors.sensors[i].mqttName));
		memset(_cfg.remoteSensors.sensors[i].mqttSlug, 0x00, sizeof(_cfg.remoteSensors.sensors[i].mqttSlug));
		memset(_cfg.remoteSensors.sensors[i].mqttTypeSlug, 0x00, sizeof(_cfg.remoteSensors.sensors[i].mqttTypeSlug));
		memset(_cfg.remoteSensors.sensors[i].mqttDeviceClass, 0x00, sizeof(_cfg.remoteSensors.sensors[i].mqttDeviceClass));
		memset(_cfg.remoteSensors.sensors[i].mqttUnit, 0x00, sizeof(_cfg.remoteSensors.sensors[i].mqttUnit));
		_cfg.remoteSensors.sensors[i].mqttIsBinary = false;
		memset(_cfg.remoteSensors.sensors[i].reserved, 0x00, sizeof(_cfg.remoteSensors.sensors[i].reserved));
	}

	_cfg.version = ConfigVersion6;
}

bool ConfigManager::save()
{
	// prepare checksum
	_cfg.version = ConfigVersion;
	_cfg.checksum = 0;
	_cfg.checksum = calcChecksum(_cfg);

	EEPROM.put(sizeof(SystemHeader), _cfg);
#if defined(ESP8266) || defined(ESP32)
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
	strncpy_P(_cfg.location.name, DefaultBoatName, ConfigMaxNameLength - 1);
	_cfg.location.name[ConfigMaxNameLength - 1] = '\0';  // Ensure null termination

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
		_cfg.relay.relays[i].pin = PinDisabled; // no pin assigned
	}

	_cfg.sdCard.csPin = PinDisabled;

	// Reset linked relay table
	for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
	{
		_cfg.relay.linkedRelays[i][0] = PinDisabled;
		_cfg.relay.linkedRelays[i][1] = PinDisabled;
	}

	// Default home page mapping: first four relays visible in order
	for (uint8_t i = 0; i < ConfigHomeButtons; ++i)
	{
		_cfg.relay.homePageMapping[i] = i; // map slot i -> relay i
	}

	_cfg.location.locationType = LocationType::Other;
	_cfg.sound.hornRelayIndex = PinDisabled; // none
	_cfg.lightSensor.nightRelayIndex = PinDisabled; // none
	_cfg.lightSensor.daytimeThreshold = 512;
	_cfg.sound.startDelayMs = 500; // 500ms


	strncpy(_cfg.location.mmsi, "000000000", ConfigMmsiLength - 1);
	_cfg.location.mmsi[ConfigMmsiLength - 1] = '\0';
	strncpy(_cfg.location.callSign, "NOCALL", ConfigCallSignLength - 1);
	_cfg.location.callSign[ConfigCallSignLength - 1] = '\0';
	strncpy(_cfg.location.homePort, "Unknown", ConfigHomePortLength - 1);
	_cfg.location.homePort[ConfigHomePortLength - 1] = '\0';
	_cfg.system.timezoneOffset = 0; // UTC
	_cfg.system.rebootOnSave = false;

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

	// Network auth defaults
	_cfg.auth.enabled = false;
	_cfg.auth.version = 0;
	_cfg.auth.apiKey[0] = '\0';
	_cfg.auth.hmacKey[0] = '\0';
	memset(_cfg.auth.reserved, 0x00, sizeof(_cfg.auth.reserved));

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
		memset(_cfg.sensors.sensors[i].pins, PinDisabled, ConfigMaxSensorPins);

	// spi pins
	_cfg.spiPins.misoPin = PinDisabled;
	_cfg.spiPins.mosiPin = PinDisabled;
	_cfg.spiPins.sckPin = PinDisabled;

	// XpdzTone defaults (pin unset; sensible tone parameters)
	_cfg.xpdzTone.pin = PinDisabled;

	// Hw479Rgb defaults (all pins unset)
	_cfg.hw479Rgb.rPin = PinDisabled;
	_cfg.hw479Rgb.gPin = PinDisabled;
	_cfg.hw479Rgb.bPin = PinDisabled;

	// RtcConfig defaults (all pins unset)
	_cfg.rtc.dataPin  = PinDisabled;
	_cfg.rtc.clockPin = PinDisabled;
	_cfg.rtc.resetPin = PinDisabled;

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