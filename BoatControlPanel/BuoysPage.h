#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BaseBoatPage.h"
#include "NextionIds.h"



class BuoysPage : public BaseBoatPage
{
private:
    uint8_t _currentBuoyIndex = 0;
    void updateBuoyDisplay();
    void cycleNextBuoy();
    void cyclePrevBuoy();
protected:
    // Required overrides
    uint8_t getPageId() const override { return PageBuoys; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;

public:
    explicit BuoysPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);
};