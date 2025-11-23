#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "Config.h"
#include "BaseBoatPage.h"
#include "NextionIds.h"
#include "BoatControlPanelConstants.h"

class SystemPage : public BaseBoatPage {
private:
    unsigned long _lastRefreshTime = 0;
	uint8_t _lastFuseBoxCpu = MaxUint8Value;
	uint16_t _lastFuseBoxMemory = MaxUint16Value;
    uint8_t _lastControlPanelCpu = MaxUint8Value;
    uint16_t _lastControlPanelMemory = MaxUint16Value;

    // Internal methods to update the display
    void updateControlPanelCpu();
    void updateControlPanelMemory();
    void updateFuseBoxCpu();
    void updateFuseBoxMemory();
    void updateAllDisplayItems();

protected:
    // Required overrides
    uint8_t getPageId() const override { return PageSystem; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void onEnterPage() override;
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleExternalUpdate(uint8_t updateType, const void* data) override;

public:
    explicit SystemPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);

    // Setters for updating values
    void setFuseBoxCpu(uint8_t cput);
    void setFuseBoxMemory(uint16_t memory);
};
