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

#include "PageSoundOvertaking.h"

#if defined(NEXTION_DISPLAY_DEVICE)



// Nextion Names/Ids on current Page
constexpr uint8_t BtnStarboard = 2; // b0
constexpr uint8_t BtnConsent = 5; // b1
constexpr uint8_t BtnPort = 3; // b2
constexpr uint8_t BtnDanger = 4; // b3
constexpr uint8_t BtnBack = 6; // b4

constexpr unsigned long RefreshIntervalMs = 10000;


PageSoundOvertaking::PageSoundOvertaking(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    SoundController* soundController)
	: BasePage(serialPort, warningMgr, commandMgrComputer), _soundController(soundController)
{
}

void PageSoundOvertaking::begin()
{

}

void PageSoundOvertaking::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void PageSoundOvertaking::handleTouch(uint8_t compId, uint8_t eventType)
{
	(void)eventType;

    switch (compId)
    {
    case BtnStarboard:
        if (_soundController)
        {
            _soundController->playSound(SoundType::OvertakeStarboard);
		}
        break;

    case BtnConsent:
        if (_soundController)
        {
            _soundController->playSound(SoundType::OvertakeConsent);
        }
        break;

    case BtnPort:
        if (_soundController)
        {
            _soundController->playSound(SoundType::OvertakePort);
        }
        break;

    case BtnDanger:
        if (_soundController)
        {
            _soundController->playSound(SoundType::OvertakeDanger);
        }
        break;

    case BtnBack:
        navigateTo(PageIdSoundSignals);
        break;

    }
}

#endif // NEXTION_DISPLAY_DEVICE
