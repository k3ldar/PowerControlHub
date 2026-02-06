#include <EEPROM.h>

#include "ConfigManager.h"
#include "SystemFunctions.h"

#if defined(BOAT_CONTROL_PANEL)
#include "ToneManager.h"
#endif

constexpr char DefaultBoatName[] = "My Boat";
constexpr char RelayNameShort[] = "R %u";
constexpr char RelayNameLong[] = "Relay %u";
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

    // Quick checks
    if (_cfg.version != ConfigVersion)
    {
        resetToDefaults();
        return false;
    }

    uint16_t expected = calcChecksum(_cfg);
    if (expected != _cfg.checksum)
    {
        // corrupted: reset to defaults
        resetToDefaults();
        return false;
    }

    return true;
}

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
    strncpy(_cfg.name, DefaultBoatName, ConfigMaxNameLength);

    // Default relay names (both short and long)
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        // Default short name: "R0", "R1", etc.
        char shortBuf[ConfigShortRelayNameLength]{ 0 };
        snprintf(shortBuf, sizeof(shortBuf), RelayNameShort, (unsigned)i);
        strncpy(_cfg.relayShortNames[i], shortBuf, ConfigShortRelayNameLength - 1);
        _cfg.relayShortNames[i][ConfigShortRelayNameLength - 1] = '\0';

        // Default long name: "Relay 0", "Relay 1", etc.
        char longBuf[ConfigLongRelayNameLength]{ 0 };
        snprintf(longBuf, sizeof(longBuf), RelayNameLong, (unsigned)i);
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

#if defined(ARDUINO_UNO_R4)
    _cfg.bluetoothEnabled = false;
    _cfg.wifiEnabled = false;
    _cfg.accessMode = 0; // 0 = AP, 1 = Client
    strncpy(_cfg.apSSID, "SmartFuseBox", sizeof(_cfg.apSSID) - 1);
    _cfg.apSSID[sizeof(_cfg.apSSID) - 1] = '\0';
	SystemFunctions::GenerateDefaultPassword(_cfg.apPassword, sizeof(_cfg.apPassword));
    _cfg.apPassword[sizeof(_cfg.apPassword) - 1] = '\0';
    _cfg.wifiPort = DefaultWifiPort;
	strncpy(_cfg.apIpAddress, DefaultApIpAddress, sizeof(_cfg.apIpAddress) - 1);
	_cfg.apIpAddress[sizeof(_cfg.apIpAddress) - 1] = '\0';
#endif

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
	_cfg.soundConfig.bad_repeatMs = 30000;

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