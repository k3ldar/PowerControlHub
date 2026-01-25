#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BaseBoatPage.h"
#include "NextionIds.h"
#include "ConfigManager.h"

class RelaySettingsPage : public BaseBoatPage
{
private:
    uint8_t _currentSettingsIndex;
    char _externalData[ConfigRelayCount + 1];
    bool validateExternalData();
    void loadConfigPage();
    void clearSelection();
protected:
    // Required overrides
    uint8_t getPageId() const override { return PageSettingsRelays; }
    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleText(const char* text) override;
public:
    explicit RelaySettingsPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);
};