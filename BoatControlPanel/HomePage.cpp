#include "HomePage.h"


// Nextion Names/Ids on current Home Page
constexpr float CompassTemperatureWarningValue = 35;
constexpr char ControlHumidity[] = "t3";
constexpr char ControlTemperature[] = "t2";
constexpr char ControlBearingText[] = "t6";
constexpr char ControlBearingDirection[] = "t4";
constexpr char ControlSpeed[] = "t5";
constexpr char ControlBoatName[] = "tBoatName";
constexpr char ControlWarning[] = "pHomeWarning";
constexpr char SpeedUnitKnots[] = "%d kn";
constexpr char BearingFormat[] = "%d°";
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

constexpr unsigned long RefreshUpdateIntervalMs = 10000;
constexpr char HomeButtonPrefix[] = "b";


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
        char cmd[10];
		snprintf(cmd, sizeof(cmd), "%s%d", HomeButtonPrefix, i);
        setPicture(cmd, ImageButtonColorGrey);
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
    if (now - _lastRefreshTime >= RefreshUpdateIntervalMs)
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

    SerialCommandManager* commandMgrComputer = getCommandMgrComputer();

    if (eventType == EventPress)
    {
        if (commandMgrComputer)
        {
			char debugMsg[64];
			snprintf(debugMsg, sizeof(debugMsg), "%s pressed", config->relayShortNames[relayIndex]);
            commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
        }
    }
    else if (eventType == EventRelease)
    {
        if (commandMgrComputer)
        {
			char debugMsg[64];
			snprintf(debugMsg, sizeof(debugMsg), "%s released", config->relayShortNames[relayIndex]);
            commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
        }

        // Toggle button state
        _buttonOn[buttonIndex] = !_buttonOn[buttonIndex];

        // Get the appropriate color based on the new state
        uint8_t newColor = getButtonColor(buttonIndex, _buttonOn[buttonIndex], ConfigHomeButtons);
        _buttonImage[buttonIndex] = newColor;

        // Update the button appearance
        char cmd[10];
        snprintf(cmd, sizeof(cmd), "%s%d", HomeButtonPrefix, buttonIndex + 1);
        setPicture(cmd, newColor);
        setPicture2(cmd, newColor);

        SerialCommandManager* commandMgrLink = getCommandMgrLink();
        if (commandMgrLink)
        {
            // Send relay command
            snprintf(cmd, sizeof(cmd), "%d%s", relayIndex, _buttonOn[buttonIndex] ? ButtonOn : ButtonOff);
            commandMgrLink->sendCommand(RelaySetState, cmd);
        }
    }
}

void HomePage::handleExternalUpdate(uint8_t updateType, const void* data)
{
	char debugMsg[64];
	snprintf(debugMsg, sizeof(debugMsg), "HomePage::handleExternalUpdate type=%u", updateType);
	SerialCommandManager* commandMgrComputer = getCommandMgrComputer();
    if (commandMgrComputer)
    {
        commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
    }

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
                char cmd[10];
                snprintf(cmd, sizeof(cmd), "%s%d", HomeButtonPrefix, buttonIndex + 1);
                setPicture(cmd, newColor);
                setPicture2(cmd, newColor);

                // Found the button, no need to continue
                break;
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
        const UInt16Update* update = static_cast<const UInt16Update*>(data);
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
        setDirection(update->value);
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::Speed) && data != nullptr)
    {
        const UInt16Update* update = static_cast<const UInt16Update*>(data);
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

void HomePage::setDirection(const char* dir)
{
    if (dir && strcmp(_lastDirection, dir) != 0)
    {
        strncpy(_lastDirection, dir, sizeof(_lastDirection) - 1);
        _lastDirection[sizeof(_lastDirection) - 1] = '\0';
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

    char cmd[10];
    dtostrf(_lastTemp, 4, 1, cmd);

    size_t len = strlen(cmd);

    // Append degree symbol (character 176) and "C"
    if (len + 2 < sizeof(cmd))  // +2 for degree symbol and 'C'
    {
        cmd[len] = (char)176;      // Degree symbol
        cmd[len + 1] = '\0';       // Null terminate
        strcat(cmd, CelsiusSuffix); // Append "C"
    }

    sendText(ControlTemperature, cmd);
}

void HomePage::updateHumidity()
{
    if (isnan(_lastHumidity))
    {
        sendText(ControlHumidity, NoValueText);
        return;
    }

    char cmd[10];
    dtostrf(_lastHumidity, 4, 1, cmd);

    size_t len = strlen(cmd);

    if (len + strlen(PercentSuffix) < sizeof(cmd))
    {
        strcpy(cmd + len, PercentSuffix);
    }

    sendText(ControlHumidity, cmd);
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
        char buffer[15];
        snprintf(buffer, sizeof(buffer), "%s%d", HomeButtonPrefix, button + 1);

        uint8_t relayIndex = config->homePageMapping[button];
        if (relayIndex <= 7)
        {
            _slotToRelay[button] = relayIndex;

            // set picture control (button image) - control names in your Nextion might differ
            setPicture(buffer, _buttonImage[button]);

            // Use short name for home page display
            sendText(buffer, config->relayShortNames[relayIndex]);
        }
        else
        {
            _slotToRelay[button] = 0xFF;
            _buttonOn[button] = false;
            _buttonImage[button] = ImageButtonColorGrey;
            setPicture(buffer, _buttonImage[button]);
            sendText(buffer, "");
        }
    }

    // Update the boat name
    sendText(ControlBoatName, config->boatName);
}
