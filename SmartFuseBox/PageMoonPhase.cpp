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

#include "PageMoonPhase.h"

#if defined(NEXTION_DISPLAY_DEVICE)


#include "Astronomy.h"
#include "DateTimeManager.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 2;
constexpr char ControlMoonPhase[] = "p2";
constexpr char ControlMoonPhaseName[] = "t1";
constexpr char ControlMoonPhaseSeaDescription[] = "t2";
constexpr char ControlMoonPhaseDescription[] = "t3";

constexpr unsigned long RefreshIntervalMs = 10000;
constexpr unsigned long AutoReturnMs = 120000;

PageMoonPhase::PageMoonPhase(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer)
    : BasePage(serialPort, warningMgr, commandMgrComputer),
	_pageEnterTime(0), _lastRefresh(0)
{
}

void PageMoonPhase::onEnterPage()
{
    // Called when page becomes active
    _pageEnterTime = millis();
    _lastRefresh = millis();
    updateMoonDisplay();
}

void PageMoonPhase::begin()
{
    updateMoonDisplay();
}

void PageMoonPhase::refresh(unsigned long now)
{
    if (now - _lastRefresh >= RefreshIntervalMs)
    {
        _lastRefresh = now;
        updateMoonDisplay();
    }

    // Auto-return to home after configured timeout
    if (_pageEnterTime != 0 && (now - _pageEnterTime) >= AutoReturnMs)
    {
        navigateTo(PageIdHome);
    }
}

void PageMoonPhase::updateMoonDisplay()
{
    MoonPhase phase = Astronomy::getMoonPhaseFromUnix(DateTimeManager::getCurrentTime());
    setPicture(ControlMoonPhase, MoonImages[static_cast<uint8_t>(phase)]);

	char buffer[BufferSizeMoonPhaseDescription];
    Astronomy::getMoonPhaseName(phase, buffer, sizeof(buffer));
    sendText(ControlMoonPhaseName, buffer);
    Astronomy::getMoonPhaseSeaDescription(phase, buffer, sizeof(buffer));
    sendText(ControlMoonPhaseSeaDescription, buffer);
    Astronomy::getMoonPhaseDescription(phase, buffer, sizeof(buffer));
	sendText(ControlMoonPhaseDescription, buffer);
}

// Handle touch events for buttons
void PageMoonPhase::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        navigateTo(PageIdHome);
        return;
    }
}

#endif // NEXTION_DISPLAY_DEVICE
