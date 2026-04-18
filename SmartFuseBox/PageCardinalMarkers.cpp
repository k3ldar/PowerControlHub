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

#include "PageCardinalMarkers.h"

#if defined(NEXTION_DISPLAY_DEVICE)


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;

PageCardinalMarkers::PageCardinalMarkers(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer)
    : BasePage(serialPort, warningMgr, commandMgrComputer)
{
}

void PageCardinalMarkers::begin()
{
}

void PageCardinalMarkers::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void PageCardinalMarkers::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonNext:
        navigateNext();
        return;

    case ButtonPrevious:
        navigatePrevious();
        return;
    }
}

#endif // NEXTION_DISPLAY_DEVICE
