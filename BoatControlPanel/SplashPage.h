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
        sendText(F("pageSplash.vaBoatName"), config->name);
        sendText(F("pageSplash.vaHomeB1"), config->relayShortNames[0]);
        sendText(F("pageSplash.vaHomeB2"), config->relayShortNames[1]);
        sendText(F("pageSplash.vaHomeB3"), config->relayShortNames[2]);
        sendText(F("pageSplash.vaHomeB4"), config->relayShortNames[3]);

        // Relay page global variables
        sendText(F("pageSplash.vaRelayName1"), config->relayLongNames[0]);
        sendText(F("pageSplash.vaRelayName2"), config->relayLongNames[1]);
        sendText(F("pageSplash.vaRelayName3"), config->relayLongNames[2]);
        sendText(F("pageSplash.vaRelayName4"), config->relayLongNames[3]);
        sendText(F("pageSplash.vaRelayName5"), config->relayLongNames[4]);
        sendText(F("pageSplash.vaRelayName6"), config->relayLongNames[5]);
        sendText(F("pageSplash.vaRelayName7"), config->relayLongNames[6]);
        sendText(F("pageSplash.vaRelayName8"), config->relayLongNames[7]);
    }

    void refresh(unsigned long now) override
    {
		(void)now;
        // No periodic refresh needed for splash page
    }
};