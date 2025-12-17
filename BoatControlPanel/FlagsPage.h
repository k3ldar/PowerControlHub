#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BaseBoatPage.h"
#include "NextionIds.h"



class FlagsPage : public BaseBoatPage
{
private:
	uint8_t _currentFlagIndex = 0;
    void updateFlagDisplay();
    void cycleNextFlag();
    void cyclePrevFlag(); 
protected:
    // Required overrides
    uint8_t getPageId() const override { return PageFlags; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;

public:
    explicit FlagsPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);
};