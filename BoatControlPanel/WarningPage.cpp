#include "WarningPage.h"
#include <NextionControl.h>

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

}

void WarningPage::begin()
{
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
}

void WarningPage::onEnterPage()
{
    // Force update when entering the page
    _lastActiveWarnings = 0;
    _lastUpdateTime = 0;
    updateWarningDisplay();
}

// Helper function to convert WarningType enum to string
String warningTypeToString(WarningType type)
{
    switch (type)
    {
    case WarningType::DefaultConfiguration:
        return F("Using Default Configuration");
    case WarningType::ConnectionLost:
        return F("Connection Lost To Fuse Box");
    case WarningType::HighCompassTemperature:
        return F("High Compass Temperature");
    case WarningType::LowBattery:
        return F("Low Battery");
    case WarningType::SensorFailure:
        return F("Sensor Failure");
    case WarningType::CompassFailure:
        return F("Compass Failure");
    case WarningType::TemperatureSensorFailure:
		return F("Temperature Sensor Failure");
    default:
        return F("Unknown Warning");
    }
}

void WarningPage::updateWarningDisplay()
{
    sendText(WarningHeader, F("System Warnings"));
    WarningManager* warningMgr = getWarningManager();

    if (!warningMgr)
        return;

    String warningText;
    bool firstWarning = true;

    // Iterate through all defined warning types (skip None)
    for (uint8_t i = 1; i < WarningCount; i++)
    {
        WarningType type = static_cast<WarningType>(i);

        if (warningMgr->isWarningActive(type))
        {
            // Add separator before subsequent warnings
            if (!firstWarning)
            {
                warningText += "\r\n";
            }
            
            warningText += warningTypeToString(type);
            firstWarning = false;
        }
    }

    // If no warnings are active, display a message
    if (warningText.length() == 0)
    {
        warningText = F("No Active Warnings");
    }

    // Send the formatted warning list to the Nextion control
    sendText(WarningListComponentName, warningText);
}

void WarningPage::refresh(unsigned long now)
{
    WarningManager* warningMgr = getWarningManager();
    if (!warningMgr)
        return;

    // Get current active warnings bitmap
    // Note: This assumes WarningManager has a method to get the bitmap
    // If not available, we can iterate through warnings to build it
    uint32_t currentWarnings = 0;
    for (uint8_t i = 1; i < WarningCount; i++)
    {
        WarningType type = static_cast<WarningType>(i);
        if (warningMgr->isWarningActive(type))
        {
            currentWarnings |= (1UL << i);
        }
    }

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