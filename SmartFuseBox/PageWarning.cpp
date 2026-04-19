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

#include "PageWarning.h"

#if defined(NEXTION_DISPLAY_DEVICE)
#include <NextionControl.h>
#include <string.h>

// Nextion Names/Ids on Warning Page
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t ButtonNext = 3;
constexpr char WarningListComponentName[] = "t1";
constexpr char WarningHeader[] = "t2";

PageWarning::PageWarning(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    MessageBus* messageBus)
    : BasePage(serialPort, warningMgr, commandMgrComputer, messageBus),
    _lastActiveWarnings(0),
    _lastUpdateTime(0)
{
    // Initialize cached text as empty
    _lastWarningText[0] = '\0';

    if (messageBus)
    {
        messageBus->subscribe<WarningChanged>([this](uint32_t) {
            if (isActive())
            {
                _lastActiveWarnings = 0;
                updateWarningDisplay();
                _lastUpdateTime = millis();
            }
        });
    }
}

void PageWarning::begin()
{
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
    _lastWarningText[0] = '\0';
}

void PageWarning::onEnterPage()
{
    // Force update when entering the page by clearing cache
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
    _lastWarningText[0] = '\0';
    updateWarningDisplay();
}

bool PageWarning::buildWarningText(char* buffer, size_t bufferSize)
{
    WarningManager* warningMgr = getWarningManager();
    if (!warningMgr)
    {
        strncpy(buffer, "No Active Warnings", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return false;
    }

    size_t bytesWritten = 0;
    bool firstWarning = true;

    // Iterate through all 32 possible bit positions in uint32_t
    for (uint8_t bit = 0; bit < 32; bit++)
    {
        WarningType type = static_cast<WarningType>(1UL << bit);

        if (warningMgr->isWarningActive(type))
        {
            // Get the warning string once
            const char* warningString = getWarningString(bit);
            
            // Skip empty/undefined warnings
            if (warningString[0] == '\0')
            {
                continue;
            }
            
            // Add separator before subsequent warnings
            if (!firstWarning)
            {
                // Check if we have room for "\r\n"
                if (bytesWritten + 2 >= bufferSize - 1)
                    break;  // Buffer full
                
                buffer[bytesWritten++] = '\r';
                buffer[bytesWritten++] = '\n';
            }
            
            // Copy warning string
            size_t warningLen = strlen(warningString);
            size_t remainingSpace = bufferSize - bytesWritten - 1;
            
            if (warningLen > remainingSpace)
                warningLen = remainingSpace;  // Truncate if needed
            
            strncpy(&buffer[bytesWritten], warningString, warningLen);
            bytesWritten += warningLen;
            
            if (bytesWritten >= bufferSize - 1)
                break;  // Buffer full
            
            firstWarning = false;
        }
    }

    // Null-terminate
    buffer[bytesWritten] = '\0';

    // If no warnings are active, display a message
    if (bytesWritten == 0)
    {
        strncpy(buffer, "No Active Warnings", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }

    return true;
}

void PageWarning::updateWarningDisplay()
{
    sendText(WarningHeader, "System Warnings");

    // Build warning text into a temporary buffer
    char newWarningText[MaxWarningTextLength];
    buildWarningText(newWarningText, MaxWarningTextLength);

    // Only send to Nextion if the text has changed
    if (strcmp(newWarningText, _lastWarningText) != 0)
    {
        sendText(WarningListComponentName, newWarningText);
        
        // Update cache
        strncpy(_lastWarningText, newWarningText, MaxWarningTextLength - 1);
        _lastWarningText[MaxWarningTextLength - 1] = '\0';
    }
}

void PageWarning::refresh(unsigned long now)
{
    WarningManager* warningMgr = getWarningManager();
    if (!warningMgr)
        return;

    // Get current active warnings bitmap directly from WarningManager
    uint32_t currentWarnings = warningMgr->getActiveWarningsMask();

    // Check if warnings changed OR 10 seconds elapsed
    bool warningsChanged = (currentWarnings != _lastActiveWarnings);
    bool timeoutElapsed = (now - _lastUpdateTime >= UpdateIntervalMs);

    if (warningsChanged || timeoutElapsed)
    {
        updateWarningDisplay();
        _lastActiveWarnings = currentWarnings;
        _lastUpdateTime = now;
    }
}

// Handle touch events for buttons
void PageWarning::handleTouch(uint8_t compId, uint8_t eventType)
{
    // Only handle release events
    if (eventType != EventRelease)
    {
        return;
    }

    // Map component ID to button index
    switch (compId)
    {
    case ButtonPrevious:
        navigatePrevious();
        break;

    case ButtonNext:
        navigateNext();
        break;
    }
}

#endif // NEXTION_DISPLAY_DEVICE
