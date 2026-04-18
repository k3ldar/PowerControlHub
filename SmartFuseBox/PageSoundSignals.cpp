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

#include "PageSoundSignals.h"

#if defined(NEXTION_DISPLAY_DEVICE)


// Nextion Names/Ids on current Page
constexpr uint8_t BtnManeuvering = 4; // b0
constexpr uint8_t BtnFog = 6; // b2
constexpr uint8_t BtnNarrowChannel = 5; // b1
constexpr uint8_t BtnEmergency = 7; // b3
constexpr uint8_t BtnOther = 8; // b4
constexpr uint8_t BtnCancelAll = 9; // b5
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;
constexpr char CancelButton[] = "b5";

constexpr unsigned long RefreshIntervalMs = 10000;


PageSoundSignals::PageSoundSignals(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    SoundController* soundController)
	: BasePage(serialPort, warningMgr, commandMgrComputer), _soundController(soundController)
{
}

void PageSoundSignals::begin()
{
    setPicture(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
    setPicture2(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
}

void PageSoundSignals::onEnterPage()
{
	if (_soundController)
    {
		updateButtonStates(_soundController->isPlaying());
    }
    _lastRefreshTime = millis();
}

void PageSoundSignals::refresh(unsigned long now)
{
    if (now - _lastRefreshTime >= RefreshIntervalMs)
    {
        _lastRefreshTime = now;
        if (_soundController)
        {
            updateButtonStates(_soundController->isPlaying());
        }
    }
}

// Handle touch events for buttons
void PageSoundSignals::handleTouch(uint8_t compId, uint8_t eventType)
{
	(void)eventType;

    switch (compId)
    {
    case BtnManeuvering:
        navigateTo(PageIdSoundManeuveringSignals);
        break;

    case BtnFog:
        navigateTo(PageIdSoundFogSignals);
        break;

    case BtnNarrowChannel:
        navigateTo(PageIdSoundOvertaking);
        break;

    case BtnEmergency:
        navigateTo(PageIdSoundEmergency);
        break;

    case BtnOther:
        navigateTo(PageIdSoundOther);
        break;

    case BtnCancelAll:
        if (_soundController)
        {
			if (_soundController->isPlaying())
            {
                _soundController->playSound(SoundType::None);
            }
            updateButtonStates(false);
        }
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
}

void PageSoundSignals::updateButtonStates(bool isActive)
{
    if (isActive)
    {
        setPicture(CancelButton, ImageButtonColorBlue + ImageButtonColorOffset);
        setPicture2(CancelButton, ImageButtonColorBlue + ImageButtonColorOffset);
    }
    else
    {
        setPicture(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
        setPicture2(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
    }

}

void PageSoundSignals::handleExternalUpdate(uint8_t updateType, const void* data)
{
    // Call base class first to handle heartbeat ACKs
    BasePage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::SoundSignal) && data != nullptr)
    {
        const BoolStateUpdate* update = static_cast<const BoolStateUpdate*>(data);
		updateButtonStates(update->value);
    }
}

#endif // NEXTION_DISPLAY_DEVICE
