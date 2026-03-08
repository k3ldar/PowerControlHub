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

#include "AboutPage.h"

// Nextion Names/Ids on current Page
constexpr uint8_t BtnNextFlag = 3; // b3
constexpr uint8_t BtnBack = 2; // b4
constexpr uint8_t BtnSetup = 7; // b0
constexpr uint8_t BtnRelaySetup = 9; // b1

constexpr unsigned long RefreshIntervalMs = 10000;

AboutPage::AboutPage(Stream* serialPort)
    : BaseDisplayPage(serialPort)
{
}

void AboutPage::begin()
{

}

void AboutPage::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void AboutPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case BtnSetup:
        setPage(PageSettings);
		break;

    case BtnRelaySetup:
        setPage(PageSettingsRelays);
        break;

    case BtnNextFlag:
		setPage(PageHome);
        break;

    case BtnBack:
        setPage(PageSystem);
        break;
    }
}
