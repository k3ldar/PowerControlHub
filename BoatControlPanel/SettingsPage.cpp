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
#include "SettingsPage.h"
#include "ConfigManager.h"
#include "SystemFunctions.h"

static const char SettingsKeyboardInputLen[] PROGMEM = "keyboardInputLen";
static const char SettingsKeyboardNumericOnly[] PROGMEM = "keyboardNumericOnly";
static const char SettingsName[] PROGMEM = "tName";
static const char SettingsError[] PROGMEM = "tError";
static const char SettingsValue[] PROGMEM = "tValue";
static const char SettingsNextButton[] PROGMEM = "b32";
static const char SettingsNext[] PROGMEM = "Next";
static const char SettingsSave[] PROGMEM = "Save";

static const char SettingsBoatName[] PROGMEM = "Boat Name";
static const char SettingsCallSign[] PROGMEM = "Call Sign";
static const char SettingsMmsiNumber[] PROGMEM = "MMSI Number";
static const char SettingsHomePort[] PROGMEM = "Home Port";
static const char ShortRelayDescription[] PROGMEM = "Short Relay Name %d";
static const char LongRelayDescription[] PROGMEM = "Long Relay Name %d";


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 4;
constexpr uint8_t ButtonNext = 29;

constexpr uint8_t ShowNumericKeyboard = 0;
constexpr uint8_t ShowAlphaKeyboard = 1;

constexpr uint8_t SettingBoatName = 0;
constexpr uint8_t SettingBoatNameLen = 30;

constexpr uint8_t SettingCallSign = 1;
constexpr uint8_t SettingCallSignLen = 10;

constexpr uint8_t SettingMmsiNumber = 2;
constexpr uint8_t SettingMmsiNumberLen = 9;

constexpr uint8_t SettingHomePortName = 3;

constexpr uint8_t SettingShortRelayOffset = 4;
constexpr uint8_t SettingShortRelay1 = 4;
constexpr uint8_t SettingShortRelay2 = 5;
constexpr uint8_t SettingShortRelay3 = 6;
constexpr uint8_t SettingShortRelay4 = 7;
constexpr uint8_t SettingShortRelay5 = 8;
constexpr uint8_t SettingShortRelay6 = 9;
constexpr uint8_t SettingShortRelay7 = 10;
constexpr uint8_t SettingShortRelay8 = 11;

constexpr uint8_t SettingLongRelayOffset = 12;
constexpr uint8_t SettingLongRelay1 = 12;
constexpr uint8_t SettingLongRelay2 = 13;
constexpr uint8_t SettingLongRelay3 = 14;
constexpr uint8_t SettingLongRelay4 = 15;
constexpr uint8_t SettingLongRelay5 = 16;
constexpr uint8_t SettingLongRelay6 = 17;
constexpr uint8_t SettingLongRelay7 = 18;
constexpr uint8_t SettingLongRelay8 = 19;


constexpr uint8_t LastSettings = 19;

SettingsPage::SettingsPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer),
    _currentSettingsIndex(SettingBoatName)
{
	_externalData[0] = '\0';
}

void SettingsPage::onEnterPage()
{
    _currentSettingsIndex = SettingBoatName;
    loadConfigPage();
}

void SettingsPage::begin()
{
    // not used just now
}

void SettingsPage::refresh(unsigned long now)
{
    //nothing to refresh currently
}

// Handle touch events for buttons
void SettingsPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        if (_currentSettingsIndex > SettingBoatName)
        {
            _currentSettingsIndex--;
        }
        break;
    case ButtonNext:
        if (validateExternalData())
        {
            _currentSettingsIndex++;
        }
        break;

    default:
        // Unknown component
		return;
    }

    loadConfigPage();
}

void SettingsPage::loadConfigPage()
{
    Config* config = getConfig();

    switch (_currentSettingsIndex)
    {
        case SettingBoatName:
    		loadPage(FPSTR(SettingsBoatName), config->name, SettingBoatNameLen, ShowAlphaKeyboard, false);
			break;

		case SettingCallSign:
            loadPage(FPSTR(SettingsCallSign), config->callSign, SettingCallSignLen, ShowAlphaKeyboard, true);
            break;

        case SettingMmsiNumber:
            loadPage(FPSTR(SettingsMmsiNumber), config->mMSI, SettingMmsiNumberLen, ShowNumericKeyboard, true);
			break;

        case SettingHomePortName:
            loadPage(FPSTR(SettingsHomePort), config->homePort, ConfigHomePortLength - 1, ShowAlphaKeyboard, true);
			break;

		case SettingShortRelay1:
		case SettingShortRelay2:
		case SettingShortRelay3:
		case SettingShortRelay4:
		case SettingShortRelay5:
		case SettingShortRelay6:
		case SettingShortRelay7:
        case SettingShortRelay8:
        {
            char buffer[20];
            uint8_t relayIndex = _currentSettingsIndex - SettingShortRelayOffset;

            // Validate array bounds
            if (relayIndex >= ConfigRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Invalid relay index"));
                return;
            }

            // Display as 1-8 instead of 3-10
            snprintf_P(buffer, sizeof(buffer), ShortRelayDescription, relayIndex + 1);

            loadPage(
                buffer,
                config->relayShortNames[relayIndex],
                ConfigShortRelayNameLength - 1,
                ShowAlphaKeyboard,
                true);

            break;
        }

		case SettingLongRelay1:
		case SettingLongRelay2:
		case SettingLongRelay3:
		case SettingLongRelay4:
		case SettingLongRelay5:
		case SettingLongRelay6:
		case SettingLongRelay7:
        case SettingLongRelay8:
        {
            char buffer[20];
            uint8_t relayIndex = _currentSettingsIndex - SettingLongRelayOffset;

            // Validate array bounds
            if (relayIndex >= ConfigRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Invalid relay index"));
                return;
            }

            snprintf_P(buffer, sizeof(buffer), LongRelayDescription, relayIndex + 1);
            loadPage(
                buffer,
                config->relayLongNames[relayIndex],
                ConfigLongRelayNameLength - 1,
                ShowAlphaKeyboard,
                true);

        break;
		}

        default:
            ConfigManager::save();
            setPage(PageAbout);
    }
}

void SettingsPage::loadPage(const char* settingName, const char* settingValue, uint8_t maxInputLength, uint8_t keyboardType, bool allowPrevious)
{
    sendValue(FPSTR(SettingsKeyboardInputLen), maxInputLength);
    sendValue(FPSTR(SettingsKeyboardNumericOnly), keyboardType);
    sendText(FPSTR(SettingsName), settingName);
    sendText(FPSTR(SettingsError), F(""));
    sendText(FPSTR(SettingsValue), settingValue);

    if (allowPrevious)
        sendCommand(F("vis b3,1"));
    else
        sendCommand(F("vis b3,0"));

    if (_currentSettingsIndex == LastSettings)
        sendText(FPSTR(SettingsNextButton), FPSTR(SettingsSave));
    else
		sendText(FPSTR(SettingsNextButton), FPSTR(SettingsNext));
}

void SettingsPage::loadPage(const __FlashStringHelper* settingName, const char* settingValue, uint8_t maxInputLength, uint8_t keyboardType, bool allowPrevious)
{
    sendValue(FPSTR(SettingsKeyboardInputLen), maxInputLength);
    sendValue(FPSTR(SettingsKeyboardNumericOnly), keyboardType);
    sendText(FPSTR(SettingsName), settingName);
    sendText(FPSTR(SettingsError), F(""));
    sendText(FPSTR(SettingsValue), settingValue);

    if (allowPrevious)
        sendCommand(F("vis b3,1"));
    else
        sendCommand(F("vis b3,0"));

    if (_currentSettingsIndex == LastSettings)
        sendText(FPSTR(SettingsNextButton), FPSTR(SettingsSave));
    else
        sendText(FPSTR(SettingsNextButton), FPSTR(SettingsNext));
}

void SettingsPage::handleText(const char* text)
{
    if (text == nullptr)
    {
        _externalData[0] = '\0';
        return;
    }
    
    if (strlen(text) >= sizeof(_externalData))
    {
        // Truncate to fit
        strncpy(_externalData, text, sizeof(_externalData) - 1);
        _externalData[sizeof(_externalData) - 1] = '\0';
    }
    else
    {
        snprintf_P(_externalData, sizeof(_externalData), PSTR("%s"), text);
    }
}

bool SettingsPage::validateExternalData()
{
    if (strlen(_externalData) == 0)
    {
        sendText(FPSTR("tError"), F("Can not be blank"));
        return false;
    }

    Config* config = getConfig();

    if (!config)
    {
        sendText(FPSTR("tError"), F("Configuration error"));
        return false;
	}

    bool result = false;

    switch (_currentSettingsIndex)
    {
    case SettingBoatName:
        strncpy(config->name, _externalData, ConfigMaxNameLength - 1);
        config->name[ConfigMaxNameLength - 1] = '\0';
        result = true;

        break;

    case SettingCallSign:
        strncpy(config->callSign, _externalData, ConfigCallSignLength - 1);
        config->callSign[ConfigCallSignLength - 1] = '\0';
        result = true;

        break;

    case SettingMmsiNumber:
        // MMSI number: must be exactly 9 digits
        if (strlen(_externalData) != 9)
        {
            sendText(FPSTR("tError"), F("MMSI must be 9 digits"));
            return false;
        }

		if (!SystemFunctions::isAllDigits(_externalData))
        {
            sendText(FPSTR("tError"), F("MMSI must be numeric"));
            return false;
        }

        strncpy(config->mMSI, _externalData, ConfigMmsiLength - 1);
        config->mMSI[ConfigMmsiLength - 1] = '\0';
        result = true;

		break;

    case SettingHomePortName:
        if (strlen(_externalData) > ConfigHomePortLength - 1)
        {
            sendText(FPSTR("tError"), F("Home port name too long"));
            return false;
        }

		strncpy(config->homePort, _externalData, ConfigHomePortLength - 1);
        config->homePort[ConfigHomePortLength - 1] = '\0';
        result = true;

		break;

    case SettingShortRelay1:
    case SettingShortRelay2:
    case SettingShortRelay3:
    case SettingShortRelay4:
    case SettingShortRelay5:
    case SettingShortRelay6:
    case SettingShortRelay7:
    case SettingShortRelay8:
    {
        if (strlen(_externalData) > ConfigShortRelayNameLength - 1)
        {
            sendText(FPSTR("tError"), F("Relay name too long"));
            return false;
        }

        uint8_t relayIndex = _currentSettingsIndex - SettingShortRelayOffset;

        if (relayIndex >= ConfigRelayCount)
        {
            sendText(FPSTR(SettingsError), F("Invalid relay index"));
            return false;
        }

        strncpy(config->relayShortNames[relayIndex], _externalData, ConfigShortRelayNameLength - 1);
        config->relayShortNames[relayIndex][ConfigShortRelayNameLength - 1] = '\0';
        result = true;

        break;
    }

    case SettingLongRelay1:
	case SettingLongRelay2:
	case SettingLongRelay3:
	case SettingLongRelay4:
	case SettingLongRelay5:
	case SettingLongRelay6:
	case SettingLongRelay7:
    case SettingLongRelay8:
    {
        if (strlen(_externalData) > ConfigLongRelayNameLength - 1)
        {
            sendText(FPSTR("tError"), F("Relay name too long"));
            return false;
        }

        uint8_t relayIndex = _currentSettingsIndex - SettingLongRelayOffset;

        if (relayIndex >= ConfigRelayCount)
        {
            sendText(FPSTR(SettingsError), F("Invalid relay index"));
            return false;
        }

        strncpy(config->relayLongNames[relayIndex], _externalData, ConfigLongRelayNameLength - 1);
        config->relayLongNames[relayIndex][ConfigLongRelayNameLength - 1] = '\0';
        result = true;

        break;
    }
    }

    if (result)
        sendText(FPSTR("tError"), F(""));

    return result;
}