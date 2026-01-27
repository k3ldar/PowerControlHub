#pragma once

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BaseBoatPage.h"
#include "NextionIds.h"



class EnvironmentPage : public BaseBoatPage
{
private:
    float _lastTemp = NAN;
	float _previousTemp = NAN;
    float _lastHumidity = NAN;
	float _previousHumidity = NAN;
    double _lastLatitude = NAN;
    double _lastLongitude = NAN;
    void updateTemperature();
    void updateHumidity();
    void updateLatLon();
	void updateSunriseSunset();
    void updateCondensation();
	void updateDewPoint();
    void updateComfortDescription(double dewPoint);

protected:
    // Required overrides
    uint8_t getPageId() const override { return PageEnvironment; }
    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleExternalUpdate(uint8_t updateType, const void* data) override;

public:
    explicit EnvironmentPage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrLink = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);
};