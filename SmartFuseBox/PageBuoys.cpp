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

#include "PageBuoys.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include "SystemFunctions.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t BtnPrevBuoy = 4;
constexpr uint8_t BtnNextBuoy = 5;
constexpr char BuoyImageName[] = "buoyImg";

constexpr uint8_t BuoyFirst = 64;
constexpr uint8_t BuoyLast = 71;

// Store buoy meanings in PROGMEM to save RAM
const char BuoyMeaning_1[] PROGMEM = "Wreck or Hazard.\r\nPlaced close to new wreck.";
const char BuoyMeaning_2[] PROGMEM = "Isolated Danger\r\nPlaced above or close to hazard.";
const char BuoyMeaning_3[] PROGMEM = "Lateral Marker Port.\r\nEdge of navigable channel.";
const char BuoyMeaning_4[] PROGMEM = "Lateral Marker Starboard.\r\nEdge of navigable channel.";
const char BuoyMeaning_5[] PROGMEM = "Preferred Channel Port.\r\nPass on port side.";
const char BuoyMeaning_6[] PROGMEM = "Preferred Channel Starboard.\r\nPass on starboard side.";
const char BuoyMeaning_7[] PROGMEM = "Safe Water.\r\nNavigable water or center line.";
const char BuoyMeaning_8[] PROGMEM = "Special Marker.\r\nSpecial area or feature.";

const char* const BuoyMeanings[8] PROGMEM = {
    BuoyMeaning_1, BuoyMeaning_2, BuoyMeaning_3, BuoyMeaning_4,
    BuoyMeaning_5, BuoyMeaning_6, BuoyMeaning_7, BuoyMeaning_8
};

constexpr unsigned long RefreshIntervalMs = 10000;


PageBuoys::PageBuoys(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer)
    : BasePage(serialPort, warningMgr, commandMgrComputer),
    _currentBuoyIndex(BuoyFirst + 1)
{
}

void PageBuoys::begin()
{
    updateBuoyDisplay();
}

void PageBuoys::refresh(unsigned long now)
{
    (void)now;
}

void PageBuoys::updateBuoyDisplay()
{
    setPicture(BuoyImageName, _currentBuoyIndex);

    uint8_t buoyIndex = _currentBuoyIndex - BuoyFirst;

    // Read the pointer from PROGMEM and convert to cha*
    const char* meaningPtr = (const char*)pgm_read_ptr(&BuoyMeanings[buoyIndex]);
    char buffer[100];
    strcpy_P(buffer, meaningPtr);

    sendText(F("buoyText"), buffer);
}

void PageBuoys::cycleNextBuoy()
{
    if (_currentBuoyIndex >= BuoyLast)
    {
        _currentBuoyIndex = BuoyFirst;
    }
    else
    {
        _currentBuoyIndex++;
    }

    updateBuoyDisplay();
}

void PageBuoys::cyclePrevBuoy()
{
    if (_currentBuoyIndex <= BuoyFirst)
    {
        _currentBuoyIndex = BuoyLast;
    }
    else
    {
        _currentBuoyIndex--;
    }

    updateBuoyDisplay();
}

// Handle touch events for buttons
void PageBuoys::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case BtnPrevBuoy:
        cyclePrevBuoy();
        break;

    case BtnNextBuoy:
        cycleNextBuoy();
        break;

    case ButtonNext:
        navigateNext();
        return;

    case ButtonPrevious:
        navigatePrevious();
        return;
    }
}

#endif // NEXTION_DISPLAY_DEVICE
