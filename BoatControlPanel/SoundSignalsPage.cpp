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
#include "SoundSignalsPage.h"


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


SoundSignalsPage::SoundSignalsPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void SoundSignalsPage::begin()
{
    setPicture(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
    setPicture2(CancelButton, ImageButtonColorGrey + ImageButtonColorOffset);
}

void SoundSignalsPage::onEnterPage()
{
    getCommandMgrLink()->sendCommand(SoundSignalActive, "");
    _lastRefreshTime = millis();
}

void SoundSignalsPage::refresh(unsigned long now)
{
    if (now - _lastRefreshTime >= RefreshIntervalMs)
    {
        _lastRefreshTime = now;
        getCommandMgrLink()->sendCommand(SoundSignalActive, "");
    }
}

// Handle touch events for buttons
void SoundSignalsPage::handleTouch(uint8_t compId, uint8_t eventType)
{
	(void)eventType;

    switch (compId)
    {
    case BtnManeuvering:
        setPage(PageSoundManeuveringSignals);
        break;

    case BtnFog:
        setPage(PageSoundFogSignals);
        break;

    case BtnNarrowChannel:
        setPage(PageSoundOvertaking);
        break;

    case BtnEmergency:
        setPage(PageSoundEmergency);
        break;

    case BtnOther:
        setPage(PageSoundOther);
        break;

    case BtnCancelAll:
		getCommandMgrLink()->sendCommand(SoundSignalCancel, "");
		getCommandMgrLink()->sendCommand(SoundSignalActive, "");
        break;

    case ButtonNext:
        setPage(PageFlags);
        return;

    case ButtonPrevious:
        setPage(PageVhfRadio);
        return;

    default:
        return;
    }
}

void SoundSignalsPage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::SoundSignal) && data != nullptr)
    {
        const BoolStateUpdate* update = static_cast<const BoolStateUpdate*>(data);

        if (update->value)
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
}
