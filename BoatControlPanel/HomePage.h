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
    char _lastDirection[5] = "";
    float _lastCompassTemp = NAN;
    double _lastLatitude = NAN;
    double _lastLongitude = NAN;
    double _displayedLatitude = NAN;
    double _displayedLongitude = NAN;
	float _lastDistance = NAN;

    byte _compassTempAboveNorm = 0;
	bool _speedInKnots = true;
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
    void updateLatLon();
    void updateDistance();
    void updateTime();
    void updateMoonPhase();
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
    void setDirection(const char* dir);
    void setCompassTemperature(float tempC);
};
