/*
 * PowerControlHub
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
#include <stdint.h>

#include "Config.h"
#include "BasePage.h"
#include "NextionIds.h"

class PageSystem : public BasePage
{
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
    uint8_t getPageId() const override { return PageIdSystem; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void onEnterPage() override;
    void handleTouch(uint8_t compId, uint8_t eventType) override;

public:
    explicit PageSystem(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer = nullptr,
        MessageBus* messageBus = nullptr);

    // Setters for updating values
    void setFuseBoxCpu(uint8_t cput);
    void setFuseBoxMemory(uint16_t memory);
};

#endif // NEXTION_DISPLAY_DEVICE
