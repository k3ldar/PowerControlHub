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

#include <NextionControl.h>
#include "NextionIds.h"
#include "BasePage.h"

class PageAbout : public BasePage
{
public:
    PageAbout(Stream* serialPort,
        WarningManager* warningMgr = nullptr,
        SerialCommandManager* commandMgrComputer = nullptr);

protected:

    uint8_t getPageId() const override { return PageIdAbout; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
};

#endif // NEXTION_DISPLAY_DEVICE
