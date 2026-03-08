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

#include "CardinalMarkersPage.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;

CardinalMarkersPage::CardinalMarkersPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void CardinalMarkersPage::begin()
{
}

void CardinalMarkersPage::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void CardinalMarkersPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonNext:
        setPage(PageBuoys);
        return;

    case ButtonPrevious:
        setPage(PageFlags);
        return;
    }
}
