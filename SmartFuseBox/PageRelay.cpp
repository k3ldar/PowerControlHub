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

#include "PageRelay.h"

#if defined(NEXTION_DISPLAY_DEVICE)


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

constexpr char RelayButtonPrefix[] = "bRelay";

constexpr unsigned long RefreshIntervalMs = 10000;


PageRelay::PageRelay(Stream* serialPort,
	WarningManager* warningMgr,
	SerialCommandManager* commandMgrComputer,
	RelayController* relayController,
	MessageBus* messageBus)
	: BasePage(serialPort, warningMgr, commandMgrComputer, messageBus), _relayController(relayController)
{
	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		_buttonImage[i] = ImageButtonColorGrey + ImageButtonColorOffset;
		_buttonImageOn[i] = ImageButtonColorBlue + ImageButtonColorOffset;
	}

	if (messageBus)
	{
		messageBus->subscribe<RelayStatusChanged>([this](uint8_t) {
			if (isActive()) updateAllButtons();
		});
	}
}

void PageRelay::begin()
{
    // If config already supplied before begin, apply it
    if (getConfig())
    {
        configUpdated();
    }

	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		char variable[10];
		snprintf_P(variable, sizeof(variable), PSTR("%s%d"), RelayButtonPrefix, i + 1);
		setPicture(variable, ImageButtonColorGrey + ImageButtonColorOffset);
		setPicture2(variable, ImageButtonColorGrey + ImageButtonColorOffset);
	}
}

void PageRelay::onEnterPage()
{
    if (getConfig())
    {
        configUpdated();
    }

    // Sync button states directly from RelayController
    updateAllButtons();
    _lastRefreshTime = millis();
}

void PageRelay::refresh(unsigned long now)
{
    // Periodically re-sync button states in case relays were changed elsewhere
    if (now - _lastRefreshTime >= RefreshIntervalMs)
    {
        _lastRefreshTime = now;
        updateAllButtons();
    }
}

// Handle touch events for buttons
void PageRelay::handleTouch(uint8_t compId, uint8_t eventType)
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }

    if (config->sound.hornRelayIndex < DefaultValue &&
        compId - 4 == config->sound.hornRelayIndex)
    {
        // relay button is configured to sound system (horn) and will be
        // controlled via own command methods from sound pages
        navigateTo(PageIdSoundSignals);
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
        navigateNext();
        return;

    case ButtonPrevious:
        navigatePrevious();
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
        if (_relayController)
        {
            bool newState = !_buttonOn[buttonIndex];
            _relayController->setRelayState(relayIndex, newState);
            updateAllButtons();
        }
    }
}

void PageRelay::configUpdated()
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
        snprintf_P(relayPrefix, sizeof(relayPrefix), PSTR("%s%d"), RelayButtonPrefix, button + 1);

        setPicture(relayPrefix, ImageButtonColorGrey + ImageButtonColorOffset);
        setPicture2(relayPrefix, ImageButtonColorGrey + ImageButtonColorOffset);

        sendText(relayPrefix, config->relay.relays[button].longName);
    }
}

void PageRelay::updateAllButtons()
{
    if (!_relayController)
        return;

    CommandResult allStates = _relayController->getAllRelayStates();

    for (uint8_t buttonIndex = 0; buttonIndex < ConfigRelayCount; ++buttonIndex)
    {
        uint8_t relayIndex = _slotToRelay[buttonIndex];
        if (relayIndex == 0xFF || relayIndex >= ConfigRelayCount)
            continue;

        bool isOn = allStates.isRelayOn(relayIndex);
        _buttonOn[buttonIndex] = isOn;

        uint8_t newColor = getButtonColor(buttonIndex, isOn, ConfigRelayCount) + ImageButtonColorOffset;
        _buttonImage[buttonIndex] = newColor;

        char buttonName[15];
        snprintf_P(buttonName, sizeof(buttonName), PSTR("%s%d"), RelayButtonPrefix, buttonIndex + 1);
        setPicture(buttonName, newColor);
        setPicture2(buttonName, newColor);
    }
}

#endif // NEXTION_DISPLAY_DEVICE
