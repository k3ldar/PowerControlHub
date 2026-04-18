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

#include "PageHome.h"

#if defined(NEXTION_DISPLAY_DEVICE)
#include "DateTimeManager.h"
#include "Astronomy.h"


// Nextion Names/Ids on current Home Page
constexpr float CompassTemperatureWarningValue = 35;
constexpr char ControlHumidity[] = "t3";
constexpr char ControlTemperature[] = "t2";
constexpr char ControlBearingText[] = "t6";
constexpr char ControlBearingDirection[] = "t4";
constexpr char ControlLonValue[] = "t5";
constexpr char ControlLatValue[] = "t7";
constexpr char ControlCurrTime[] = "t8";
constexpr char ControlDistance[] = "t10";
constexpr char ControlSpeed[] = "tSpeed";
constexpr char ControlBoatName[] = "tBoatName";
constexpr char ControlWarning[] = "pHomeWarning";
constexpr char ControlMoonPhase[] = "pHomeMoon";
const char SpeedUnitKnots[] PROGMEM = "%d kn/h";
const char SpeedUnitKilometer[] PROGMEM = "%d km/h";
const char DistanceUnitKnots[] PROGMEM = "%s nm";
const char DistanceUnitKilometer[] PROGMEM = "%s km";
const char BearingFormat[] PROGMEM = "%d°";

constexpr char HomeButtonPrefix[] = "b";

constexpr uint8_t Button1 = 1; // bHomeRelay1
constexpr uint8_t Button2 = 2; // bHomeRelay2
constexpr uint8_t Button3 = 3; // bHomeRelay3
constexpr uint8_t Button4 = 4; // bHomeRelay4
constexpr uint8_t ButtonTemperature = 6; //t2
constexpr uint8_t ButtonHumidity = 7; //t3
constexpr uint8_t ButtonSpeed = 9; //tSpeed / swap between kn/km
constexpr uint8_t ButtonVhf = 10; //p0
constexpr uint8_t ButtonNext = 12;
constexpr uint8_t ButtonWarning = 13;
constexpr uint8_t ButtonMoon = 23;
constexpr uint8_t ButtonIdOffset = 1; // Offset to map button IDs to array indices

constexpr unsigned long RefreshUpdateIntervalMs = 10000;
constexpr double GpsNoiseThresholdDegrees = 0.00001;


PageHome::PageHome(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    RelayController* relayController)
	: BasePage(serialPort, warningMgr, commandMgrComputer), _relayController(relayController)
{
    
}

void PageHome::begin()
{
    // If config already supplied before begin, apply it
    if (getConfig())
    {
        configUpdated();
    }

	for (uint8_t i = 1; i <= ConfigHomeButtons; ++i)
	{
		char cmd[15];
		snprintf_P(cmd, sizeof(cmd), PSTR("%sHomeRelay%d"), HomeButtonPrefix, i);
		setPicture(cmd, ImageButtonColorGrey);
	}
    _compassTempAboveNorm = 0;
}

void PageHome::onEnterPage()
{
    if (getConfig())
    {
        configUpdated();
    }
    
    // Sync button states directly from RelayController
    updateAllButtons();
    _lastRefreshTime = millis();

    updateAllDisplayItems();
}

void PageHome::refresh(unsigned long now)
{
    updateAllDisplayItems();

    // Periodically re-sync button states in case relays were changed elsewhere
    if (now - _lastRefreshTime >= RefreshUpdateIntervalMs)
    {
        _lastRefreshTime = now;
        updateAllButtons();
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
            // Reset internal state to NaN to prevent flickering
            _lastTemp = NAN;
            _lastHumidity = NAN;
            
            sendText(ControlHumidity, NoValueText);
            sendText(ControlTemperature, NoValueText);
        }
    }
}

void PageHome::updateAllDisplayItems()
{
    updateTemperature();
    updateHumidity();
    updateBearing();
    updateSpeed();
	updateDirection();
    updateLatLon();
    updateDistance();
    updateTime();
    updateMoonPhase();
}

// Handle touch events for buttons
void PageHome::handleTouch(uint8_t compId, uint8_t eventType)
{
    Config* config = getConfig();

    if (!config)
    {
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
            buttonIndex = compId - ButtonIdOffset;  // Get slot index (0-3)
            
            // Check if THIS button's mapped relay is the horn
            if (config->sound.hornRelayIndex < DefaultValue &&
                _slotToRelay[buttonIndex] == config->sound.hornRelayIndex)
            {
                // This button controls the horn, go to sound page
                navigateTo(PageIdSoundSignals);
                return;
            }

            break;  // Continue to normal relay button handling

		case ButtonTemperature:
		case ButtonHumidity:
			navigateTo(PageIdEnvironment);
			return;

        case ButtonSpeed:
            _speedInKnots = !_speedInKnots;
			updateSpeed();
            return;

		case ButtonVhf:
			navigateTo(PageIdVhfRadio);
			return;

        case ButtonNext: 
            navigateNext();
            return;

        case ButtonMoon:
            navigateTo(PageIdMoonPhases);
            return;

        case ButtonWarning:
            navigateTo(PageIdWarning);
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
			snprintf_P(debugMsg, sizeof(debugMsg), PSTR("%s pressed"), config->relay.relays[relayIndex].shortName);
			commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
		}
    }
    else if (eventType == EventRelease)
    {
		if (commandMgrComputer)
		{
			char debugMsg[64];
			snprintf_P(debugMsg, sizeof(debugMsg), PSTR("%s released"), config->relay.relays[relayIndex].shortName);
			commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
		}

        // Toggle button state
        _buttonOn[buttonIndex] = !_buttonOn[buttonIndex];

        // Get the appropriate color based on the new state
        uint8_t newColor = getButtonColor(buttonIndex, _buttonOn[buttonIndex], ConfigHomeButtons);
        _buttonImage[buttonIndex] = newColor;

        // Update the button appearance
        char cmd[15];
        snprintf_P(cmd, sizeof(cmd), PSTR("%sHomeRelay%d"), HomeButtonPrefix, buttonIndex + 1);
        setPicture(cmd, newColor);
        setPicture2(cmd, newColor);

		if (_relayController)
		{
			_relayController->setRelayState(relayIndex, _buttonOn[buttonIndex]);
		}
    }
}

void PageHome::handleExternalUpdate(uint8_t updateType, const void* data)
{
	char debugMsg[64];
	snprintf_P(debugMsg, sizeof(debugMsg), PSTR("HomePage::handleExternalUpdate type=%u"), updateType);
	SerialCommandManager* commandMgrComputer = getCommandMgrComputer();
    if (commandMgrComputer)
    {
        commandMgrComputer->sendDebug(debugMsg, F("HomePage"));
    }

    // Call base class first to handle heartbeat ACKs
    BasePage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::RelayState))
    {
        updateAllButtons();
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
    else if (updateType == static_cast<uint8_t>(PageUpdateType::GpsLatitude) && data != nullptr)
    {
        // FloatStateUpdate is used for GPS lat/lon notifications
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        _lastLatitude = update->value;
        updateLatLon();
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::GpsLongitude) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        _lastLongitude = update->value;
        updateLatLon();
    }
	else if (updateType == static_cast<uint8_t>(PageUpdateType::GpsDistance) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        _lastDistance = update->value;
        updateDistance();
    }
}

// --- Public setters ---
void PageHome::setTemperature(float tempC)
{
    if (isnan(_lastTemp) || _lastTemp != tempC)
    {
        _lastTemp = tempC;
        updateTemperature();
    }
}

void PageHome::setHumidity(float humPerc)
{
    if (isnan(_lastHumidity) || _lastHumidity != humPerc)
    {
        _lastHumidity = humPerc;
        updateHumidity();
    }
}

void PageHome::setBearing(float dir)
{
    if (isnan(_lastBearing) || _lastBearing != dir)
    {
        _lastBearing = dir;
        updateBearing();
    }
}

void PageHome::setSpeed(float speedKn)
{
    if (isnan(_lastSpeed) || _lastSpeed != speedKn)
    {
        _lastSpeed = speedKn;
        updateSpeed();
    }
}

void PageHome::setDirection(const char* dir)
{
    if (dir && strcmp(_lastDirection, dir) != 0)
    {
        strncpy(_lastDirection, dir, sizeof(_lastDirection) - 1);
        _lastDirection[sizeof(_lastDirection) - 1] = '\0';
        updateDirection();
    }
}

void PageHome::setCompassTemperature(float tempC)
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
void PageHome::updateAllButtons()
{
    if (!_relayController)
        return;

    CommandResult allStates = _relayController->getAllRelayStates();

    for (uint8_t buttonIndex = 0; buttonIndex < ConfigHomeButtons; ++buttonIndex)
    {
        uint8_t relayIndex = _slotToRelay[buttonIndex];
        if (relayIndex == 0xFF || relayIndex >= ConfigRelayCount)
            continue;

        bool isOn = allStates.isRelayOn(relayIndex);
        _buttonOn[buttonIndex] = isOn;
        _buttonImage[buttonIndex] = getButtonColor(buttonIndex, isOn, ConfigHomeButtons);

        char cmd[15];
        snprintf_P(cmd, sizeof(cmd), PSTR("%sHomeRelay%d"), HomeButtonPrefix, buttonIndex + 1);
        setPicture(cmd, _buttonImage[buttonIndex]);
        setPicture2(cmd, _buttonImage[buttonIndex]);
    }
}

void PageHome::updateTemperature()
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

void PageHome::updateHumidity()
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

void PageHome::updateBearing()
{
    if (isnan(_lastBearing))
    {
        sendText(ControlBearingText, NoValueText);
        return;
    }

    char buffer[10];
    // Use explicit single-byte degree symbol (0xB0) to avoid UTF-8 two-byte sequence
    snprintf_P(buffer, sizeof(buffer), PSTR("%d%c"), (int)_lastBearing, (char)176);
    sendText(ControlBearingText, buffer);
}

void PageHome::updateSpeed()
{
    if (isnan(_lastSpeed))
    {
        _lastSpeed = 0;
    }

    char buffer[10];
    float displaySpeed = _lastSpeed;
    const char* format = SpeedUnitKilometer;

    if (_speedInKnots)
    {
        // Convert km/h to knots (1 km/h = 0.539957 knots)
        displaySpeed = _lastSpeed * 0.539957f;
        format = SpeedUnitKnots;
    }

    snprintf_P(buffer, sizeof(buffer), format, (int)displaySpeed);
    sendText(ControlSpeed, buffer);
}

void PageHome::updateLatLon()
{
    if (isnan(_lastLatitude))
    {
        sendText(ControlLatValue, NoValueText);
        _displayedLatitude = NAN;
    }
    else
    {
        // Only update display if change is significant or first time
        bool shouldUpdate = isnan(_displayedLatitude) || 
                           fabs(_lastLatitude - _displayedLatitude) > GpsNoiseThresholdDegrees;
        
        if (shouldUpdate)
        {
            _displayedLatitude = _lastLatitude;
            
            char latBuf[20];
            dtostrf(_lastLatitude, 10, 6, latBuf);
            char* pLat = latBuf;

            while (*pLat == ' ') 
                pLat++;
            
            sendText(ControlLatValue, pLat);
        }
    }

    if (isnan(_lastLongitude))
    {
        sendText(ControlLonValue, NoValueText);
        _displayedLongitude = NAN;
    }
    else
    {
        // Only update display if change is significant or first time
        bool shouldUpdate = isnan(_displayedLongitude) || 
                           fabs(_lastLongitude - _displayedLongitude) > GpsNoiseThresholdDegrees;
        
        if (shouldUpdate)
        {
            _displayedLongitude = _lastLongitude;
            
            char lonBuf[20];
            dtostrf(_lastLongitude, 10, 6, lonBuf);

            char* pLon = lonBuf;

            while (*pLon == ' ')
                pLon++;

            sendText(ControlLonValue, pLon);
        }
    }
}

void PageHome::updateDistance()
{
    if (isnan(_lastDistance))
    {
        _lastDistance = 0.00;
    }

    char buffer[16];
    float displayDistance = _lastDistance;
    const char* format = DistanceUnitKilometer;

    if (_speedInKnots)
    {
        // Convert km to knots (1 km = 0.539957 knots)
        displayDistance = _lastDistance * 0.539957f;
        format = DistanceUnitKnots;
    }

    char num[10];
    dtostrf(displayDistance, 0, 1, num);
    snprintf_P(buffer, sizeof(buffer), format, num);
    sendText(ControlDistance, buffer);
}

void PageHome::updateTime()
{
    char buf[DateTimeBufferLength + 5];
    if (!DateTimeManager::formatDateTimeReadable(buf, sizeof(buf)))
    {
        sendText(ControlCurrTime, NoValueText);
        return;
    }

    char timeBuf[13]; 
    if (strlen(buf) >= 19)
    {
        memcpy(timeBuf, buf + 11, 8);
        timeBuf[8] = '\0';
        strcat(timeBuf, " UTC");
    }
    else
    {
        // fallback: use whatever is available and append UTC
        strncpy(timeBuf, buf, sizeof(timeBuf) - 5);
        timeBuf[sizeof(timeBuf) - 5] = '\0';
        strcat(timeBuf, " UTC");
    }

    sendText(ControlCurrTime, timeBuf);
}

void PageHome::updateMoonPhase()
{
	MoonPhase phase = Astronomy::getMoonPhaseFromUnix(DateTimeManager::getCurrentTime());
    setPicture(ControlMoonPhase, MoonImages[static_cast<uint8_t>(phase)]);
}

void PageHome::updateDirection()
{
    sendText(ControlBearingDirection, _lastDirection);
}

void PageHome::configUpdated()
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
        snprintf_P(buffer, sizeof(buffer), PSTR("%sHomeRelay%d"), HomeButtonPrefix, button + 1);

        uint8_t relayIndex = config->relay.homePageMapping[button];
        if (relayIndex <= 7)
        {
            _slotToRelay[button] = relayIndex;

            // set picture control (button image) - control names in your Nextion might differ
            setPicture(buffer, _buttonImage[button]);

            // Use short name for home page display
            sendText(buffer, config->relay.relays[relayIndex].shortName);
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
    sendText(ControlBoatName, config->location.name);
}

#endif // NEXTION_DISPLAY_DEVICE
