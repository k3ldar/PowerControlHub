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
        sendCommand(String(F("pageSplash.vaBoatName.txt=\"")) + String(config->boatName) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaHomeB1.txt=\"")) + String(config->relayShortNames[0]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaHomeB2.txt=\"")) + String(config->relayShortNames[1]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaHomeB3.txt=\"")) + String(config->relayShortNames[2]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaHomeB4.txt=\"")) + String(config->relayShortNames[3]) + String(F("\"")));

        // Relay page global variables
        sendCommand(String(F("pageSplash.vaRelayName1.txt=\"")) + String(config->relayLongNames[0]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName2.txt=\"")) + String(config->relayLongNames[1]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName3.txt=\"")) + String(config->relayLongNames[2]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName4.txt=\"")) + String(config->relayLongNames[3]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName5.txt=\"")) + String(config->relayLongNames[4]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName6.txt=\"")) + String(config->relayLongNames[5]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName7.txt=\"")) + String(config->relayLongNames[6]) + String(F("\"")));
        sendCommand(String(F("pageSplash.vaRelayName8.txt=\"")) + String(config->relayLongNames[7]) + String(F("\"")));
    }

    void refresh(unsigned long now) override
    {
		(void)now;
        // No periodic refresh needed for splash page
    }
};