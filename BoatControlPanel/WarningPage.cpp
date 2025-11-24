#include "WarningPage.h"
#include <NextionControl.h>
#include <string.h>

// Nextion Names/Ids on Warning Page
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t ButtonNext = 3;
constexpr char WarningListComponentName[] = "t1";
constexpr char WarningHeader[] = "t2";

WarningPage::WarningPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer),
    _lastActiveWarnings(0),
    _lastUpdateTime(0)
{
    // Initialize cached text as empty
    _lastWarningText[0] = '\0';
}

void WarningPage::begin()
{
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
    _lastWarningText[0] = '\0';
}

void WarningPage::onEnterPage()
{
    // Force update when entering the page by clearing cache
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
    _lastWarningText[0] = '\0';
    updateWarningDisplay();
}

bool WarningPage::buildWarningText(char* buffer, size_t bufferSize)
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

void WarningPage::updateWarningDisplay()
{
    sendText(WarningHeader, F("System Warnings"));

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

void WarningPage::refresh(unsigned long now)
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
void WarningPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    // Only handle release events
    if (eventType != EventRelease) {
        return;
    }

    // Map component ID to button index
    switch (compId)
    {
    case ButtonPrevious:
        setPage(PageHome);
        break;

    case ButtonNext:
        setPage(PageRelay);
        break;
    }
}

void WarningPage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    // If warning state changed, force immediate update
    if (updateType == static_cast<uint8_t>(PageUpdateType::Warning))
    {
        // Reset the cached state to force an update
        _lastActiveWarnings = 0;
        updateWarningDisplay();
        _lastUpdateTime = millis();
    }
}