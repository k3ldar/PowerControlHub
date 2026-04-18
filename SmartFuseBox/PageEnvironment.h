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

#include "Local.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <stdint.h>

#include "BasePage.h"
#include "NextionIds.h"



class PageEnvironment : public BasePage
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
    uint8_t getPageId() const override { return PageIdEnvironment; }
    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleExternalUpdate(uint8_t updateType, const void* data) override;

public:
    explicit PageEnvironment(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer = nullptr);
};

#endif // NEXTION_DISPLAY_DEVICE
