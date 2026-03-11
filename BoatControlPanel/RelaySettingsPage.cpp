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
#include "RelaySettingsPage.h"
#include "SystemFunctions.h"

static const char SettingsError[] PROGMEM = "tRelayError";
static const char SettingSelectedText[] PROGMEM = "tSelected";
static const char PropertyButtonColor[] PROGMEM = "bco";
static const char SettingRelayButton1[] PROGMEM = "b0";
static const char SettingRelayButton2[] PROGMEM = "b1";
static const char SettingRelayButton3[] PROGMEM = "b2";
static const char SettingRelayButton4[] PROGMEM = "b4";
static const char SettingRelayButton5[] PROGMEM = "b5";
static const char SettingRelayButton6[] PROGMEM = "b6";
static const char SettingRelayButton7[] PROGMEM = "b7";
static const char SettingRelayButton8[] PROGMEM = "b8";
static const char* const  SettingRelayButtons[] PROGMEM = {
    SettingRelayButton1, SettingRelayButton2, SettingRelayButton3,
    SettingRelayButton4, SettingRelayButton5, SettingRelayButton6,
    SettingRelayButton7, SettingRelayButton8
};

// global variable of page that is currently being configured
static const char SettingsRelaySetupId[] PROGMEM = "relaySetupId";
// global variable for maximum number of relays to select
static const char SettingsMaxSelectionCount[] PROGMEM = "relayMaximumSelectionCount";


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 3;
constexpr uint8_t ButtonNext = 2;

constexpr uint16_t ButtonSelected = 5990;
constexpr uint16_t ButtonNotSelected = 65535;

// home page button selection
constexpr uint8_t relayHomePageSetupId = 0;

// always on button selection
constexpr uint8_t relayDefaultOn = 1;

// allows user to have a dedicated horn relay
constexpr uint8_t relayHornRelay = 2;

// associate button selection, two relays can be associated together so 
// on together and off together, you can have two of these associations
constexpr uint8_t relayAssociate1 = 3;
constexpr uint8_t relayAssociate2 = 4;

//page allowing users to select custom colors for each relay button that is on
constexpr uint8_t relayColorSelection = 5;

// page allowing user to select night relay 
constexpr uint8_t relayNightRelay = 6;

RelaySettingsPage::RelaySettingsPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer),
    _currentSettingsIndex(relayHomePageSetupId)
{
    _externalData[0] = '\0';
}

void RelaySettingsPage::onEnterPage()
{
	Config* config = getConfig();

    if (!config)
        return;

	for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
        sendText(reinterpret_cast<const __FlashStringHelper*>(buttonName), config->relayShortNames[i]);
    }

    _currentSettingsIndex = relayHomePageSetupId;
    loadConfigPage();
}

void RelaySettingsPage::begin()
{
    // not used just now
}

void RelaySettingsPage::refresh(unsigned long now)
{
    (void)now;
    //nothing to refresh currently
}

// Handle touch events for buttons
void RelaySettingsPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        if (_currentSettingsIndex > relayHomePageSetupId)
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

void RelaySettingsPage::loadConfigPage()
{
    Config* config = getConfig();

    if (!config)
        return;

	clearSelection();

    switch (_currentSettingsIndex)
    {
        case relayHomePageSetupId:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayHomePageSetupId);

            // Load the 4 selected relay indices and set their button colors
            for (uint8_t i = 0; i < ConfigHomeButtons; i++)
            {
                uint8_t relayIndex = config->homePageMapping[i];
                
                if (relayIndex < ConfigRelayCount)
                {
                    const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[relayIndex]));
                    setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                        FPSTR(PropertyButtonColor), ButtonSelected);
                }
                
                _externalData[i] = '0' + relayIndex;
            }

            _externalData[ConfigHomeButtons] = '\0';
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }

        case relayDefaultOn:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayDefaultOn);
            uint8_t selectionCount = 0;

            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                bool selButton = config->defaulRelayState[i];
                uint16_t buttonColor = selButton ? ButtonSelected : ButtonNotSelected;
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));

                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);

                if (selButton)
                {
                    _externalData[selectionCount] = '0' + i;
                    selectionCount++;
                }
            }

            _externalData[selectionCount] = '\0';
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }

        case relayHornRelay:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayHornRelay);

            uint8_t hornIndex = config->hornRelayIndex;
            
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                uint16_t buttonColor = (hornIndex == i) ? ButtonSelected : ButtonNotSelected;
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);
            }

            // Populate _externalData with current selection
            if (hornIndex < ConfigRelayCount)
            {
                _externalData[0] = '0' + hornIndex;
                _externalData[1] = '\0';
            }
            else
            {
                _externalData[0] = '\0';
            }
            
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }

        case relayAssociate1:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayAssociate1);

            uint8_t selectionCount = 0;

            // Iterate through all 8 relay buttons
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                bool isSelected = false;
                
                // Check if this relay is in the linked pair
                for (uint8_t j = 0; j < ConfigLinkededRelayCount; j++)
                {
                    if (config->linkedRelays[0][j] == i && config->linkedRelays[0][j] < ConfigRelayCount)
                    {
                        isSelected = true;
                        _externalData[selectionCount] = '0' + i;
                        selectionCount++;
                        break;
                    }
                }

                uint16_t buttonColor = isSelected ? ButtonSelected : ButtonNotSelected;
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);
            }

            _externalData[selectionCount] = '\0';
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }

        case relayAssociate2:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayAssociate2);

            uint8_t selectionCount = 0;

            // Iterate through all 8 relay buttons
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                bool isSelected = false;
                
                // Check if this relay is in the linked pair
                for (uint8_t j = 0; j < ConfigLinkededRelayCount; j++)
                {
                    if (config->linkedRelays[1][j] == i && config->linkedRelays[1][j] < ConfigRelayCount)
                    {
                        isSelected = true;
                        _externalData[selectionCount] = '0' + i;
                        selectionCount++;
                        break;
                    }
                }

                uint16_t buttonColor = isSelected ? ButtonSelected : ButtonNotSelected;
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);
            }

            _externalData[selectionCount] = '\0';
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }

        case relayColorSelection:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayColorSelection);

            // Load current button image selections and set button colors
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                uint8_t imageId = config->buttonImage[i];
                
                // Map image ID to character (1-5, default to white)
                char colorChar;
                uint16_t buttonColor;
                
                switch (imageId)
                {
                    case ImageButtonColorBlue:
                        colorChar = '1';
                        buttonColor = 1055;
                        break;
                    case ImageButtonColorGreen:
                        colorChar = '2';
                        buttonColor = 2024;
                        break;
                    case ImageButtonColorOrange:
                        colorChar = '3';
                        buttonColor = 64520;
                        break;
                    case ImageButtonColorRed:
                        colorChar = '4';
                        buttonColor = 63488;
                        break;
                    case ImageButtonColorYellow:
                        colorChar = '5';
                        buttonColor = 65504;
                        break;
                    default:
                        colorChar = '0';
                        buttonColor = 65535;
                        break;
                }
                
                _externalData[i] = colorChar;
                
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);
            }

            _externalData[ConfigRelayCount] = '\0';
            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }
        case relayNightRelay:
        {
            sendValue(FPSTR(SettingsRelaySetupId), relayNightRelay);

            uint8_t nightRelayIdx = config->lightSensor.nightRelayIndex;

            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                uint16_t buttonColor = (nightRelayIdx == i) ? ButtonSelected : ButtonNotSelected;
                const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
                setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
                    FPSTR(PropertyButtonColor), buttonColor);
            }

            if (nightRelayIdx < ConfigRelayCount)
            {
                _externalData[0] = '0' + nightRelayIdx;
                _externalData[1] = '\0';
            }
            else
            {
                _externalData[0] = '\0';
            }

            sendText(FPSTR(SettingSelectedText), _externalData);

            break;
        }
        default:
            ConfigManager::save();
            setPage(PageAbout);
    }
}

void RelaySettingsPage::handleText(const char* text)
{
    if (text == nullptr)
    {
        _externalData[0] = '\0';
        return;
    }

    if (strlen(text) >= sizeof(_externalData))
    {
        strncpy(_externalData, text, sizeof(_externalData) - 1);
        _externalData[sizeof(_externalData) - 1] = '\0';
    }
    else
    {
        snprintf_P(_externalData, sizeof(_externalData), PSTR("%s"), text);
    }
}

bool RelaySettingsPage::validateExternalData()
{
    Config* config = getConfig();

    if (!config)
    {
        sendText(FPSTR(SettingsError), F("Configuration error"));
        return false;
    }

    bool result = false;

    switch (_currentSettingsIndex)
    {
        case relayHomePageSetupId:
        {
            // Validate we have exactly ConfigHomeButtons characters
            size_t len = strlen(_externalData);
            if (len != ConfigHomeButtons)
            {
                sendText(FPSTR(SettingsError), F("Must select 4 relays"));
                return false;
            }

            // Validate each character is a valid relay index (0-7)
            for (uint8_t i = 0; i < ConfigHomeButtons; i++)
            {
                if (_externalData[i] < '0' || _externalData[i] > '7')
                {
                    sendText(FPSTR(SettingsError), F("Invalid relay index"));
                    return false;
                }
            }

            // Convert ASCII characters to numeric values and store
            for (uint8_t i = 0; i < ConfigHomeButtons; i++)
            {
                config->homePageMapping[i] = _externalData[i] - '0';
            }

            result = true;
            break;
        }

        case relayDefaultOn:
        {
            size_t len = strlen(_externalData);

            // Can have 0 to ConfigRelayCount selections
            if (len > ConfigRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Too many selections"));
                return false;
            }

            // Validate each character is a valid relay index (0-7) and check for duplicates
            for (uint8_t i = 0; i < len; i++)
            {
                if (_externalData[i] < '0' || _externalData[i] > '7')
                {
                    sendText(FPSTR(SettingsError), F("Invalid relay index"));
                    return false;
                }

                // Check for duplicates
                for (uint8_t j = i + 1; j < len; j++)
                {
                    if (_externalData[i] == _externalData[j])
                    {
                        sendText(FPSTR(SettingsError), F("Duplicate selection"));
                        return false;
                    }
                }
            }

            // Clear all default states first
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                config->defaulRelayState[i] = false;
            }

            // Set selected relays to ON by default
            for (uint8_t i = 0; i < len; i++)
            {
                uint8_t relayIndex = _externalData[i] - '0';
                config->defaulRelayState[relayIndex] = true;
            }

            result = true;
            break;
        }

        case relayHornRelay:
        {
            // Allow empty selection (no horn relay)
            if (strlen(_externalData) == 0)
            {
                config->hornRelayIndex = 0xFF; // none
                result = true;
                break;
            }

            // Validate single character
            if (strlen(_externalData) != 1 ||
                _externalData[0] < '0' || _externalData[0] > '7')
            {
                sendText(FPSTR(SettingsError), F("Select 0 or 1 relay"));
                return false;
            }

            config->hornRelayIndex = _externalData[0] - '0';
            result = true;
            break;
        }

        case relayAssociate1:
        {
            size_t len = strlen(_externalData);

            // Allow 0 or exactly 2 selections
            if (len != 0 && len != ConfigLinkededRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Select 0 or 2 relays"));
                return false;
            }

            // Validate relay indices and check for duplicates
            for (uint8_t i = 0; i < len; i++)
            {
                if (_externalData[i] < '0' || _externalData[i] > '7')
                {
                    sendText(FPSTR(SettingsError), F("Invalid relay index"));
                    return false;
                }

                for (uint8_t j = i + 1; j < len; j++)
                {
                    if (_externalData[i] == _externalData[j])
                    {
                        sendText(FPSTR(SettingsError), F("Duplicate selection"));
                        return false;
                    }
                }
            }

            // Store selections or clear if none
            if (len == 0)
            {
                // Clear linked relays
                for (uint8_t i = 0; i < ConfigLinkededRelayCount; i++)
                {
                    config->linkedRelays[0][i] = 0xFF;
                }
            }
            else
            {
                for (uint8_t i = 0; i < ConfigLinkededRelayCount; i++)
                {
                    config->linkedRelays[0][i] = _externalData[i] - '0';
                }
            }

            result = true;
            break;
        }

        case relayAssociate2:
        {
            size_t len = strlen(_externalData);

            // Allow 0 or exactly 2 selections
            if (len != 0 && len != ConfigLinkededRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Select 0 or 2 relays"));
                return false;
            }

            // Validate relay indices and check for duplicates
            for (uint8_t i = 0; i < len; i++)
            {
                if (_externalData[i] < '0' || _externalData[i] > '7')
                {
                    sendText(FPSTR(SettingsError), F("Invalid relay index"));
                    return false;
                }

                for (uint8_t j = i + 1; j < len; j++)
                {
                    if (_externalData[i] == _externalData[j])
                    {
                        sendText(FPSTR(SettingsError), F("Duplicate selection"));
                        return false;
                    }
                }
            }

            // Store selections or clear if none
            if (len == 0)
            {
                // Clear linked relays
                for (uint8_t i = 0; i < ConfigLinkededRelayCount; i++)
                {
                    config->linkedRelays[1][i] = 0xFF;
                }
            }
            else
            {
                for (uint8_t i = 0; i < ConfigLinkededRelayCount; i++)
                {
                    config->linkedRelays[1][i] = _externalData[i] - '0';
                }
            }

            result = true;
            break;
        }

        case relayColorSelection:
        {
            // Must have exactly 8 characters (one color per relay)
            size_t len = strlen(_externalData);
            if (len != ConfigRelayCount)
            {
                sendText(FPSTR(SettingsError), F("Must select all 8 colors"));
                return false;
            }

            // Convert color characters to image IDs and store
            for (uint8_t i = 0; i < ConfigRelayCount; i++)
            {
                uint8_t imageId;
                switch (_externalData[i])
                {
                    case '1': imageId = ImageButtonColorBlue;   break;
                    case '2': imageId = ImageButtonColorGreen;  break;
                    case '3': imageId = ImageButtonColorOrange; break;
                    case '4': imageId = ImageButtonColorRed;    break;
                    case '5': imageId = ImageButtonColorYellow; break;
                    default:  imageId = ImageButtonColorGrey;   break;
                }
                
                config->buttonImage[i] = imageId;
            }

            result = true;
            break;
        }

        case relayNightRelay:
        {
            if (strlen(_externalData) == 0)
            {
                config->lightSensor.nightRelayIndex = 0xFF;
                result = true;
                break;
            }

            if (strlen(_externalData) != 1 ||
                _externalData[0] < '0' || _externalData[0] > '7')
            {
                sendText(FPSTR(SettingsError), F("Select 0 or 1 relay"));
                return false;
            }

            config->lightSensor.nightRelayIndex = _externalData[0] - '0';
            result = true;
            break;
        }
    }

    if (result)
        sendText(FPSTR(SettingsError), F(""));

    return result;
}

void RelaySettingsPage::clearSelection()
{
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        const char* buttonName = reinterpret_cast<const char*>(pgm_read_ptr(&SettingRelayButtons[i]));
        setComponentProperty(reinterpret_cast<const __FlashStringHelper*>(buttonName),
            FPSTR(PropertyButtonColor), ButtonNotSelected);
    }
}