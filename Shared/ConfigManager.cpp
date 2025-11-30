#include <EEPROM.h>

#include "ConfigManager.h"

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
    strncpy(_cfg.boatName, DefaultBoatName, ConfigMaxBoatNameLength);

    // Default relay names (both short and long)
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        // Default short name: "R0", "R1", etc.
        char shortBuf[ConfigShortRelayName]{ 0 };
        snprintf(shortBuf, sizeof(shortBuf), RelayNameShort, (unsigned)i);
        strncpy(_cfg.relayShortNames[i], shortBuf, ConfigShortRelayName - 1);
        _cfg.relayShortNames[i][ConfigShortRelayName - 1] = '\0';

        // Default long name: "Relay 0", "Relay 1", etc.
        char longBuf[ConfigLongRelayName]{ 0 };
        snprintf(longBuf, sizeof(longBuf), RelayNameLong, (unsigned)i);
        strncpy(_cfg.relayLongNames[i], longBuf, ConfigLongRelayName - 1);
        _cfg.relayLongNames[i][ConfigLongRelayName - 1] = '\0';
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
	
#if defined(ARDUINO_UNO_R4)
    _cfg.bluetoothEnabled = true;
    _cfg.wifiEnabled = true;
    _cfg.accessMode = 0; // 0 = AP, 1 = Client
    _cfg._apSSID[0] = '\0';
    _cfg._apPassword[0] = '\0';
	_cfg.port = 80;
#endif

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