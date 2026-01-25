#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BaseBoatPage.h"
#include "NextionIds.h"

constexpr uint8_t MaximumDataLength = 31; // max length for external data incl null terminator

class SettingsPage : public BaseBoatPage
{
private:
	uint8_t _currentSettingsIndex;
    char _externalData[MaximumDataLength];
    bool validateExternalData();
    void loadPage(const char* settingName, const char* settingValue, uint8_t maxInputLength, uint8_t keyboardType, bool allowPrevious);
    void loadPage(const __FlashStringHelper* settingName, const char* settingValue, uint8_t maxInputLength, uint8_t keyboardType, bool allowPrevious);
    void loadConfigPage();
protected:
    // Required overrides
    uint8_t getPageId() const override { return PageSettings; }
    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleText(const char* text) override;
public:
    explicit SettingsPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);
};