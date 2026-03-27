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
#pragma once

#include "NextionIds.h"
#include <NextionControl.h>
#include "ConfigManager.h"

class SplashPage : public BaseDisplayPage
{
public:
    SplashPage(Stream* serialPort)
		: BaseDisplayPage(serialPort)
    {}

    uint8_t getPageId() const override { return PageSplash; }

    void onEnterPage() override
    {
		// No special actions needed on entering splash page
    }

    void begin() override
    {
        Config* config = ConfigManager::getConfigPtr();

        if (!config)
        {
            return;
        }

        // initialize global variables
        sendText(F("pageSplash.vaBoatName"), config->vessel.name);
        sendText(F("pageSplash.vaHomeB1"), config->relay.relays[0].shortName);
        sendText(F("pageSplash.vaHomeB2"), config->relay.relays[1].shortName);
        sendText(F("pageSplash.vaHomeB3"), config->relay.relays[2].shortName);
        sendText(F("pageSplash.vaHomeB4"), config->relay.relays[3].shortName);

        // Relay page global variables
        sendText(F("pageSplash.vaRelayName1"), config->relay.relays[0].longName);
        sendText(F("pageSplash.vaRelayName2"), config->relay.relays[1].longName);
        sendText(F("pageSplash.vaRelayName3"), config->relay.relays[2].longName);
        sendText(F("pageSplash.vaRelayName4"), config->relay.relays[3].longName);
        sendText(F("pageSplash.vaRelayName5"), config->relay.relays[4].longName);
        sendText(F("pageSplash.vaRelayName6"), config->relay.relays[5].longName);
        sendText(F("pageSplash.vaRelayName7"), config->relay.relays[8].longName);
        sendText(F("pageSplash.vaRelayName8"), config->relay.relays[7].longName);
    }

    void refresh(unsigned long now) override
    {
		(void)now;
        // No periodic refresh needed for splash page
    }
};