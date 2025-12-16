#include "RelayPage.h"


// Nextion Names/Ids on current Home Page
constexpr uint8_t Button1 = 4; // b1
constexpr uint8_t Button2 = 5; // b2
constexpr uint8_t Button3 = 6; // b3
constexpr uint8_t Button4 = 7; // b4
constexpr uint8_t Button5 = 8; // b5
constexpr uint8_t Button6 = 9; // b6
constexpr uint8_t Button7 = 10; // b7
constexpr uint8_t Button8 = 11; // b8
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t ButtonIdOffset = 4; // Offset to map button IDs to array indices

constexpr char ButtonOn[] = "1";
constexpr char ButtonOff[] = "0";

constexpr char RelayButtonPrefix[] = "bRelay";

constexpr unsigned long RefreshIntervalMs = 10000;


RelayPage::RelayPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        _buttonImage[i] = ImageButtonColorGrey + ImageButtonColorOffset;
        _buttonImageOn[i] = ImageButtonColorBlue + ImageButtonColorOffset;
	}
}

void RelayPage::begin()
{
    // If config already supplied before begin, apply it
    if (getConfig())
    {
        configUpdated();
    }

    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        char variable[10];
        snprintf(variable, sizeof(variable), "%s%d", RelayButtonPrefix, i + 1);
        setPicture(variable, ImageButtonColorGrey + ImageButtonColorOffset);
        setPicture2(variable, ImageButtonColorGrey + ImageButtonColorOffset);
	}
}

void RelayPage::onEnterPage()
{
    if (getConfig())
    {
        configUpdated();
    }

    // Request relay states to update button states
    getCommandMgrLink()->sendCommand(RelayRetrieveStates, "");
    _lastRefreshTime = millis();
}

void RelayPage::refresh(unsigned long now)
{
    // Send R2 command every 10 seconds to refresh relay states
    if (now - _lastRefreshTime >= RefreshIntervalMs)
    {
        getCommandMgrComputer()->sendDebug(F("Sending R2"), F("RelayPage"));
        _lastRefreshTime = now;
        getCommandMgrLink()->sendCommand(RelayRetrieveStates, "");
    }
}

// Handle touch events for buttons
void RelayPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }

    if (config->hornRelayIndex < DefaultValue &&
        compId - 4 == config->hornRelayIndex)
    {
        // relay button is configured to sound system (horn) and will be
        // controlled via own command methods from sound pages
        setPage(PageSoundSignals);
        return;
    }

    // Map component ID to button index
    uint8_t buttonIndex = InvalidButtonIndex;

    switch (compId)
    {
    case Button1:
    case Button2:
    case Button3:
    case Button4:
    case Button5:
    case Button6:
    case Button7:
    case Button8:
        buttonIndex = compId - ButtonIdOffset;
        break;

    case ButtonNext:
        setPage(PageSoundSignals);
        return;

    case ButtonPrevious:
        setPage(PageHome);
        return;

    default:
        return;
    }

    // Check if we have a valid config and the button is mapped to a relay
    if (buttonIndex >= ConfigRelayCount)
    {
        return;
    }

    uint8_t relayIndex = _slotToRelay[buttonIndex];

    // Check if this button slot has a valid relay mapping
    if (relayIndex == 0xFF || relayIndex >= ConfigRelayCount)
    {
        return;
    }

    if (eventType == EventRelease)
    {
        // Send relay command
        SerialCommandManager* commandMgrLink = getCommandMgrLink();
        if (commandMgrLink)
        {
            // R3 to update relay status in fuse box
            bool newState = !_buttonOn[buttonIndex];

            // R3 to update relay status in fuse box
            char buffer[12];
            snprintf(buffer, sizeof(buffer), "%u", relayIndex);
            
            StringKeyValue param;
            strncpy(param.key, buffer, DefaultMaxParamKeyLength);
            param.key[DefaultMaxParamKeyLength] = '\0';
            strncpy(param.value, (newState ? ButtonOn : ButtonOff), DefaultMaxParamValueLength);
            param.value[DefaultMaxParamValueLength] = '\0';
            
            commandMgrLink->sendCommand(RelaySetState, "", "", &param, 1);

            // send R4 to get relay state, to confirm change and reflect any failures etc,
            // this will prevent the UI being in an incorrect state if the command fails
            param.value[0] = '\0';
            commandMgrLink->sendCommand(RelayStatusGet, "", "", &param, 1);
        }
    }
}

void RelayPage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::RelayState) && data != nullptr)
    {
        const RelayStateUpdate* update = static_cast<const RelayStateUpdate*>(data);

        // Find if this relay is mapped to any button on this page
        for (uint8_t buttonIndex = 0; buttonIndex < ConfigRelayCount; ++buttonIndex)
        {
            if (_slotToRelay[buttonIndex] == update->relayIndex)
            {
                // Update internal state
                _buttonOn[buttonIndex] = update->isOn;

                // Get the appropriate color for the new state
                uint8_t newColor = getButtonColor(buttonIndex, update->isOn, ConfigRelayCount);
                newColor += ImageButtonColorOffset;
                _buttonImage[buttonIndex] = newColor;

                // Update the button appearance on display
                char buttonName[15];
                snprintf(buttonName, sizeof(buttonName), "%s%d", RelayButtonPrefix, buttonIndex);
                setPicture(buttonName, newColor);
                setPicture2(buttonName, newColor);

                // Log the update for debugging (using Long name)
                SerialCommandManager* commandMgrComputer = getCommandMgrComputer();
                if (commandMgrComputer)
                {
					commandMgrComputer->sendDebug(F("Relay State Updated"), F("RelayPage"));
                }

                break;
            }
        }
    }
}

void RelayPage::configUpdated()
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }

    for (uint8_t button = 0; button < ConfigRelayCount; ++button)
    {
        _slotToRelay[button] = button;

        // Initialize button to OFF state (grey)
        _buttonOn[button] = false;
        _buttonImage[button] = ImageButtonColorGrey + ImageButtonColorOffset;

        char relayPrefix[15];
        snprintf(relayPrefix, sizeof(relayPrefix), "%s%d", RelayButtonPrefix, button);

        setPicture(relayPrefix, ImageButtonColorGrey + ImageButtonColorOffset);
        setPicture2(relayPrefix, ImageButtonColorGrey + ImageButtonColorOffset);

        sendText(relayPrefix, config->relayLongNames[button]);
    }
}
