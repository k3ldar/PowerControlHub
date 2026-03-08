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
// 
// 
// 

#include "SoundOvertakingPage.h"




// Nextion Names/Ids on current Page
constexpr uint8_t BtnStarboard = 2; // b0
constexpr uint8_t BtnConsent = 5; // b1
constexpr uint8_t BtnPort = 3; // b2
constexpr uint8_t BtnDanger = 4; // b3
constexpr uint8_t BtnBack = 6; // b4

constexpr unsigned long RefreshIntervalMs = 10000;


SoundOvertakingPage::SoundOvertakingPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void SoundOvertakingPage::begin()
{

}

void SoundOvertakingPage::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void SoundOvertakingPage::handleTouch(uint8_t compId, uint8_t eventType)
{
	(void)eventType;

    switch (compId)
    {
    case BtnStarboard:
        getCommandMgrLink()->sendCommand(SoundSignalOvertakeStarboard, "");
        break;

    case BtnConsent:
        getCommandMgrLink()->sendCommand(SoundSignalOvertakeConsent, "");
        break;

    case BtnPort:
        getCommandMgrLink()->sendCommand(SoundSignalOvertakePort, "");
        break;

    case BtnDanger:
        getCommandMgrLink()->sendCommand(SoundSignalOvertakeDanger, "");
        break;

    case BtnBack:
        setPage(PageSoundSignals);
        break;

    }
}
