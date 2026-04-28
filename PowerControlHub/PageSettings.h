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

constexpr uint8_t MaximumDataLength = 31; // max length for external data incl null terminator

class PageSettings : public BasePage
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
    uint8_t getPageId() const override { return PageIdSettings; }
    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleText(const char* text) override;
public:
    explicit PageSettings(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer = nullptr);
};

#endif // NEXTION_DISPLAY_DEVICE
