#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "Config.h"
#include "BaseBoatPage.h"
#include "NextionIds.h"
#include "BoatControlPanelConstants.h"

class HomePage : public BaseBoatPage {
private:
	unsigned long _lastRefreshTime = 0;
    float _lastTemp = NAN;
    float _lastHumidity = NAN;
    float _lastBearing = NAN;
    float _lastSpeed = NAN;
    String _lastDirection = "";
    float _lastCompassTemp = NAN;
    byte _compassTempAboveNorm = 0;
	bool _buttonOn[ConfigHomeButtons] = { false, false, false, false };
	byte _buttonImage[ConfigHomeButtons] = { ImageButtonColorGrey, ImageButtonColorGrey, ImageButtonColorGrey, ImageButtonColorGrey };
	const byte _buttonImageOn[ConfigHomeButtons] = { ImageButtonColorBlue, ImageButtonColorBlue, ImageButtonColorBlue, ImageButtonColorBlue };

    uint8_t _slotToRelay[ConfigHomeButtons] = { 0xFF, 0xFF, 0xFF, 0xFF }; // map home slots 0..3 -> relay index 0..7 or 0xFF empty

    // Internal methods to update the display
    void updateTemperature();
    void updateHumidity();
    void updateBearing();
    void updateSpeed();
    void updateDirection();
    void updateAllDisplayItems();

protected:
    // Required overrides
    uint8_t getPageId() const override { return PageHome; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
	void onEnterPage() override;
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleExternalUpdate(uint8_t updateType, const void* data) override;

public:
    explicit HomePage(Stream* serialPort,
                     WarningManager* warningMgr,
                     SerialCommandManager* commandMgrLink = nullptr,
                     SerialCommandManager* commandMgrComputer = nullptr);

    // Override configUpdated from BaseBoatPage
    void configUpdated() override;

    // Setters for updating values
    void setTemperature(float tempC);
    void setHumidity(float humPerc);
    void setBearing(float dir);
    void setSpeed(float speedKn);
    void setDirection(String dir);
    void setCompassTemperature(float tempC);
};
