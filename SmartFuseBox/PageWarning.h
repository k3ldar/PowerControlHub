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

#include <Arduino.h>
#include <BaseDisplayPage.h>
#include "BasePage.h"

class PageWarning : public BasePage
{
private:
    uint32_t _lastActiveWarnings;      // Cache of last active warnings bitmap
    unsigned long _lastUpdateTime;      // Timestamp of last display update
    static const unsigned long UpdateIntervalMs = 10000; // 10 seconds
    
    // Cache for warning text to prevent unnecessary Nextion updates
    static constexpr size_t MaxWarningTextLength = 256;  // Adjust based on your needs
    char _lastWarningText[MaxWarningTextLength];
    
    // Shared function to update warning display
    void updateWarningDisplay();
    
    // Helper to build warning text into a buffer
    bool buildWarningText(char* buffer, size_t bufferSize);
    
public:
    PageWarning(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer);

    void begin() override;
    void onEnterPage() override;
    void refresh(unsigned long now) override;
    void handleTouch(uint8_t compId, uint8_t eventType) override;
    void handleExternalUpdate(uint8_t updateType, const void* data) override;
    
    uint8_t getPageId() const override { return PageIdWarning; }
};

#endif // NEXTION_DISPLAY_DEVICE
