#include "HomePage.h"


// Nextion Names/Ids on current Home Page
constexpr float CompassTemperatureWarningValue = 35;
constexpr char ControlHumidity[] = "t3";
constexpr char ControlTemperature[] = "t2";
constexpr char ControlBearingText[] = "t6";
constexpr char ControlBearingDirection[] = "t4";
constexpr char ControlSpeed[] = "t5";
constexpr char ControlBoatName[] = "t0";
constexpr char ControlWarning[] = "p2";
constexpr char NoValueText[] = "--";
constexpr char SpeedUnitKnots[] = "%d kn";
constexpr char BearingFormat[] = "%d°";
constexpr char PercentSuffix[] = "%";
constexpr char CelsiusSuffix[] = "C";
constexpr char ButtonOn[] = "=1";
constexpr char ButtonOff[] = "=0";


constexpr uint8_t Button1 = 1; // b1
constexpr uint8_t Button2 = 2; // b2
constexpr uint8_t Button3 = 3; // b3
constexpr uint8_t Button4 = 4; // b4
constexpr uint8_t ButtonNext = 12;
constexpr uint8_t ButtonWarning = 13;
constexpr uint8_t ButtonIdOffset = 1; // Offset to map button IDs to array indices

constexpr unsigned long RefreshIntervalMs = 10000;


HomePage::HomePage(Stream* serialPort,
                   WarningManager* warningMgr,
                   SerialCommandManager* commandMgrLink,
                   SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
    
}

void HomePage::begin()
{
    // If config already supplied before begin, apply it
    if (getConfig())
    {
        configUpdated();
    }

    for (uint8_t i = 1; i <= ConfigHomeButtons; ++i)
    {
        setPicture(ButtonPrefix + String(i), ImageButtonColorGrey);
	}
    _compassTempAboveNorm = 0;
}

void HomePage::onEnterPage()
{
    if (getConfig())
    {
        configUpdated();
    }
    
    // Request relay states to update button states
    getCommandMgrLink()->sendCommand(RelayRetrieveStates, "");
    _lastRefreshTime = millis();

    updateAllDisplayItems();
}

void HomePage::refresh(unsigned long now)
{
    updateAllDisplayItems();
    // Send R2 command every 10 seconds to refresh relay states
    if (now - _lastRefreshTime >= RefreshIntervalMs)
    {
        getCommandMgrComputer()->sendDebug(F("Sending R2"), F("HomePage"));
        _lastRefreshTime = now;
        getCommandMgrLink()->sendCommand(RelayRetrieveStates, "");
    }
    
    // Update warning display
    WarningManager* warningMgr = getWarningManager();
    if (warningMgr)
    {
        if (warningMgr->hasWarnings())
        {
            setPicture(ControlWarning, ImageWarning);
        }
        else
        {
            setPicture(ControlWarning, ImageBlank);
        }
        
        // Update connection-related displays
        if (warningMgr->isWarningActive(WarningType::ConnectionLost) || warningMgr->isWarningActive(WarningType::TemperatureSensorFailure))
        {
            sendText(ControlHumidity, NoValueText);
            sendText(ControlTemperature, NoValueText);
        }
    }
}

void HomePage::updateAllDisplayItems()
{
    updateTemperature();
    updateHumidity();
    updateBearing();
    updateSpeed();
	updateDirection();
}

// Handle touch events for buttons
void HomePage::handleTouch(uint8_t compId, uint8_t eventType)
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }


    if (config->hornRelayIndex < DefaultValue &&
        compId - 1 == config->hornRelayIndex)
    {
        // relay button is configured to sound system (horn) and will be
        // controlled via own command methods from sound pages
        setPage(PageSoundSignals);
        return;
    }


    // Map component ID to button index
    uint8_t buttonIndex = InvalidButtonIndex;
    switch (compId)
    {
        case Button1:
        case Button2:
        case Button3:
        case Button4:
            buttonIndex = compId - ButtonIdOffset;
            break;

        case ButtonNext: 
            setPage(PageRelay);
            return;

        case ButtonWarning:
            setPage(PageWarning);
            return;  

        default:
            return;
    }

    // Check if we have a valid config and the button is mapped to a relay
    if (buttonIndex >= ConfigHomeButtons)
        return;

    uint8_t relayIndex = _slotToRelay[buttonIndex];

    // Check if this button slot has a valid relay mapping
    if (relayIndex == 0xFF || relayIndex >= ConfigRelayCount)
        return;

    // Get the relay short name from config (for home page display)
    String relayName = String(config->relayShortNames[relayIndex]);

    SerialCommandManager* commandMgrComputer = getCommandMgrComputer();

    if (eventType == EventPress)
    {
        if (commandMgrComputer)
        {
            commandMgrComputer->sendDebug(relayName + String(F(" pressed")), F("HomePage"));
        }
    }
    else if (eventType == EventRelease)
    {
        if (commandMgrComputer)
        {
            commandMgrComputer->sendDebug(relayName + String(F(" released")), F("HomePage"));
        }

        // Toggle button state
        _buttonOn[buttonIndex] = !_buttonOn[buttonIndex];

        // Get the appropriate color based on the new state
        uint8_t newColor = getButtonColor(buttonIndex, _buttonOn[buttonIndex], ConfigHomeButtons);
        _buttonImage[buttonIndex] = newColor;

        // Update the button appearance
        setPicture(ButtonPrefix + String(buttonIndex + 1), newColor);
        setPicture2(ButtonPrefix + String(buttonIndex + 1), newColor);

        // Send relay command
        String cmd = String(relayIndex) + (_buttonOn[buttonIndex] ? ButtonOn : ButtonOff);
        SerialCommandManager* commandMgrLink = getCommandMgrLink();
        if (commandMgrLink)
        {
            commandMgrLink->sendCommand(RelaySetState, cmd);
        }
    }
}

void HomePage::handleText(String text)
{
    (void)text;
    // nothing to handle here
}

void HomePage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    getCommandMgrComputer()->sendDebug("HomePage::handleExternalUpdate type=" + String(updateType), F("HomePage"));
    
    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::RelayState) && data != nullptr)
    {
        const RelayStateUpdate* update = static_cast<const RelayStateUpdate*>(data);

        // Find if this relay is mapped to any button on this page
        for (uint8_t buttonIndex = 0; buttonIndex < ConfigHomeButtons; ++buttonIndex)
        {
            if (_slotToRelay[buttonIndex] == update->relayIndex)
            {
                
                // Update internal state
                _buttonOn[buttonIndex] = update->isOn;

                // Get the appropriate color for the new state
                uint8_t newColor = getButtonColor(buttonIndex, update->isOn, ConfigHomeButtons);
                _buttonImage[buttonIndex] = newColor;

                // Update the button appearance on display
                String buttonName = ButtonPrefix + String(buttonIndex + 1);
                setPicture(buttonName, newColor);
                setPicture2(buttonName, newColor);

                // Log the update for debugging (using short name)
                Config* config = getConfig();
                String relayName = config ? String(config->relayShortNames[update->relayIndex]) : String(update->relayIndex);

                break; // Found the button, no need to continue
            }
        }
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Temperature) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        setTemperature(update->value);
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Humidity) && data != nullptr)
    {
        const IntStateUpdate* update = static_cast<const IntStateUpdate*>(data);
        setHumidity(static_cast<float>(update->value));
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Bearing) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        setBearing(update->value);
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Direction) && data != nullptr)
    {
        const CharStateUpdate* update = static_cast<const CharStateUpdate*>(data);
        setDirection(String(update->value));
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Speed) && data != nullptr)
    {
        const IntStateUpdate* update = static_cast<const IntStateUpdate*>(data);
        setSpeed(static_cast<float>(update->value));
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::CompassTemp) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        setCompassTemperature(update->value);
    }
}

// --- Public setters ---
void HomePage::setTemperature(float tempC)
{
    if (isnan(_lastTemp) || _lastTemp != tempC)
    {
        _lastTemp = tempC;
        updateTemperature();
    }
}

void HomePage::setHumidity(float humPerc)
{
    if (isnan(_lastHumidity) || _lastHumidity != humPerc)
    {
        _lastHumidity = humPerc;
        updateHumidity();
    }
}

void HomePage::setBearing(float dir)
{
    if (isnan(_lastBearing) || _lastBearing != dir)
    {
        _lastBearing = dir;
        updateBearing();
    }
}

void HomePage::setSpeed(float speedKn)
{
    if (isnan(_lastSpeed) || _lastSpeed != speedKn)
    {
        _lastSpeed = speedKn;
        updateSpeed();
    }
}

void HomePage::setDirection(String dir)
{
    if (_lastDirection == "" || _lastDirection != dir)
    {
        _lastDirection = dir;
        updateDirection();
    }
}

void HomePage::setCompassTemperature(float tempC)
{
    if (_lastCompassTemp != tempC)
    {
        _lastCompassTemp = tempC;

        if (tempC > CompassTemperatureWarningValue && _compassTempAboveNorm < 10)
        {
            _compassTempAboveNorm++;

            if (_compassTempAboveNorm > 5)
                getWarningManager()->raiseWarning(WarningType::HighCompassTemperature);
        }
        else
        {
            _compassTempAboveNorm = 0;
            
            getWarningManager()->clearWarning(WarningType::HighCompassTemperature);
        }
    }
}

// --- Private update methods ---
void HomePage::updateTemperature()
{
    if (isnan(_lastTemp))
    {
        sendText(ControlTemperature, NoValueText);
        return;
    }

    sendText(ControlTemperature, String(_lastTemp, 1) + String((char)176) + CelsiusSuffix); // one decimal place 
}

void HomePage::updateHumidity()
{
    if (isnan(_lastHumidity))
    {
        sendText(ControlHumidity, NoValueText);
        return;
    }

    sendText(ControlHumidity, String(_lastHumidity, 1) + PercentSuffix);
}

void HomePage::updateBearing()
{
    if (isnan(_lastBearing))
    {
        sendText(ControlBearingText, NoValueText);
        return;
    }

    char buffer[10];
    snprintf(buffer, sizeof(buffer), BearingFormat, (int)_lastBearing);
    sendText(ControlBearingText, buffer);
}

void HomePage::updateSpeed()
{
    if (isnan(_lastSpeed))
    {
        sendText(ControlSpeed, NoValueText);
        return;
    }

    char buffer[10];
    snprintf(buffer, sizeof(buffer), SpeedUnitKnots, (int)_lastSpeed);
    sendText(ControlSpeed, buffer);
}

void HomePage::updateDirection()
{
    sendText(ControlBearingDirection, _lastDirection);
}

void HomePage::configUpdated()
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }

    // update Nextion with config details
    // Example: apply home page mapping and enabled mask to UI slots
    for (uint8_t button = 0; button < ConfigHomeButtons; ++button)
    {
        uint8_t relayIndex = config->homePageMapping[button];
        if (relayIndex <= 7)
        {
            _slotToRelay[button] = relayIndex;

            // set picture control (button image) - control names in your Nextion might differ
            setPicture(ButtonPrefix + String(button + 1), _buttonImage[button]);

            // Use short name for home page display
            String shortName = String(config->relayShortNames[relayIndex]);
            sendText(String(ButtonPrefix) + String(button + 1), shortName);
        }
        else
        {
            _slotToRelay[button] = 0xFF;
            _buttonOn[button] = false;
            _buttonImage[button] = ImageButtonColorGrey;
            setPicture(ButtonPrefix + String(button + 1), _buttonImage[button]);
            sendText(String(ButtonPrefix) + String(button + 1), "");
        }
    }

    // Update the boat name
    String boatName = String(config->boatName);
    sendText(ControlBoatName, boatName);
}
