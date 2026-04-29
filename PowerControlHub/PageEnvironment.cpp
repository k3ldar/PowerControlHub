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

#include "PageEnvironment.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include "Environment.h"
#include "DateTimeManager.h"
#include "SunCalculator.h"
#include "SystemFunctions.h"

const char TextLowRisk[] PROGMEM = "Low";
const char TextWatch[] PROGMEM = "Watch";
const char TextHighRisk[] PROGMEM = "High";
const char TextNA[] PROGMEM = "N/A";
const char TextColdDamp[] PROGMEM = "Cold & Damp";
const char TextCold[] PROGMEM = "Cold";
const char TextWarmSticky[] PROGMEM = "Warm & Sticky";
const char TextHumid[] PROGMEM = "Humid";
const char TextComfortable[] PROGMEM = "Comfortable";
const char TextFair[] PROGMEM = "Fair";

constexpr char ControlSunriseTime[] = "t2";
constexpr char ControlSunsetTime[] = "t4";
constexpr char ControlTemperature[] = "t8";
constexpr char ControlTemperatureIndicator[] = "p5";
constexpr char ControlComfort[] = "t9";
constexpr char ControlComfortIndicator[] = "p3";
constexpr char ControlHumidity[] = "t5";
constexpr char ControlHumidityIndicator[] = "p4";
constexpr char ControlDewPoint[] = "t10";
constexpr char ControlDewPointIndicator[] = "p1";
constexpr char ControlCondensation[] = "t14";
constexpr char ControlCondensationIndicator[] = "p0";

// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 2;


PageEnvironment::PageEnvironment(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    MessageBus* messageBus)
    : BasePage(serialPort, warningMgr, commandMgrComputer, messageBus)
{
    if (messageBus)
    {
        messageBus->subscribe<TemperatureUpdated>([this](float newTemp) {
            if (!isActive()) return;
            _previousTemp = _lastTemp;
            _lastTemp = newTemp;
            updateTemperature();
        });
        messageBus->subscribe<HumidityUpdated>([this](uint8_t newHumidity) {
            if (!isActive()) return;
            _previousHumidity = _lastHumidity;
            _lastHumidity = static_cast<float>(newHumidity);
            updateHumidity();
        });
        messageBus->subscribe<GpsLocationUpdated>([this](double lat, double lon) {
            if (!isActive()) return;
            _lastLatitude = lat;
            _lastLongitude = lon;
            updateLatLon();
        });
    }
}

void PageEnvironment::onEnterPage()
{
	updateHumidity();
	updateTemperature();
	updateLatLon();
}

void PageEnvironment::begin()
{
    // not used just now
}

void PageEnvironment::refresh(unsigned long now)
{
    (void)now;

}

// Handle touch events for buttons
void PageEnvironment::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        navigatePrevious();
        return;
    }
}

void PageEnvironment::updateTemperature()
{
    if (isnan(_lastTemp))
    {
        sendText(ControlTemperature, NoValueText);
		setPicture(ControlTemperatureIndicator, ImageQuestionMark);
        return;
    }

    char cmd[14];
    dtostrf(_lastTemp, 4, 1, cmd);

    size_t len = strlen(cmd);

    // Append degree symbol (character 176) and "C"
    if (len + 2 < sizeof(cmd))
    {
        cmd[len] = (char)176;
        cmd[len + 1] = '\0';
        strcat(cmd, CelsiusSuffix);
    }

	// update direction indicator increasing/decreasing/same but only within 0.5 degree change
    if (_lastTemp > _previousTemp)
    {
        setPicture(ControlTemperatureIndicator, ImageUpArrow);
    }
    else if (_previousTemp > _lastTemp)
    {
        setPicture(ControlTemperatureIndicator, ImageDownArrow);
    }
    else
    {
        setPicture(ControlTemperatureIndicator, ImageInwardArrow);
    }

    sendText(ControlTemperature, cmd);
    updateCondensation();
    updateDewPoint();
}

void PageEnvironment::updateHumidity()
{
    if (isnan(_lastHumidity))
    {
        sendText(ControlHumidity, NoValueText);
		setPicture(ControlHumidityIndicator, ImageQuestionMark);
        return;
    }

    char cmd[10];
    dtostrf(_lastHumidity, 4, 1, cmd);

    size_t len = strlen(cmd);

    if (len + strlen(PercentSuffix) < sizeof(cmd))
    {
        strcpy(cmd + len, PercentSuffix);
    }

    // update direction indicator increasing/decreasing/same
	if (_lastHumidity > _previousHumidity)
    {
        setPicture(ControlHumidityIndicator, ImageUpArrow);
    }
    else if (_previousHumidity > _lastHumidity)
    {
        setPicture(ControlHumidityIndicator, ImageDownArrow);
    }
    else
    {
        setPicture(ControlHumidityIndicator, ImageInwardArrow);
    }

    sendText(ControlHumidity, cmd);
    updateCondensation();
    updateDewPoint();
}

void PageEnvironment::updateLatLon()
{
    if (isnan(_lastLatitude) || isnan(_lastLongitude))
    {
        return;
    }

    updateSunriseSunset();
}

void PageEnvironment::updateSunriseSunset()
{
	Config* config = getConfig();

	if (config == nullptr)
    {
        return;
    }

    SunTimes suntimes = SunCalculator::calculateSunTimes(_lastLatitude, _lastLongitude,
        DateTimeManager::getYear(),
        DateTimeManager::getMonth(),
        DateTimeManager::getDay(),
		config->system.timezoneOffset);

    char cmd[32];

	if (!suntimes.isValid)
    {
		SystemFunctions::progmemToBuffer(TextNA, cmd, sizeof(cmd));
        sendText(ControlSunriseTime, cmd);
        sendText(ControlSunsetTime, cmd);
        return;
    }


    // Wrap hours to 0-23 range
    int sunriseHour = (int)suntimes.sunrise;
    if (sunriseHour < 0) sunriseHour += 24;
    if (sunriseHour >= 24) sunriseHour -= 24;

    int sunriseMin = (int)((suntimes.sunrise - (int)suntimes.sunrise) * 60);
    if (sunriseMin < 0) sunriseMin += 60;

    snprintf_P(cmd, sizeof(cmd), PSTR("%02d:%02d"), sunriseHour, sunriseMin);
    sendText(ControlSunriseTime, cmd);

    int sunsetHour = (int)suntimes.sunset;
    if (sunsetHour < 0) sunsetHour += 24;
    if (sunsetHour >= 24) sunsetHour -= 24;

    int sunsetMin = (int)((suntimes.sunset - (int)suntimes.sunset) * 60);
    if (sunsetMin < 0) sunsetMin += 60;

    snprintf_P(cmd, sizeof(cmd), PSTR("%02d:%02d"), sunsetHour, sunsetMin);
    sendText(ControlSunsetTime, cmd);
}

void PageEnvironment::updateCondensation()
{
    double dewPt = Environment::dewPoint(_lastTemp, _lastHumidity);
    CondensationRisk risk = Environment::condensationRisk(_lastTemp, dewPt, false);
    char buffer[24];

    switch (risk)
    {
    case CondensationRisk::Low:
        setPicture(ControlCondensationIndicator, ImageGreenCircle);
		SystemFunctions::progmemToBuffer(TextLowRisk, buffer, sizeof(buffer));
        break;
    case CondensationRisk::Watch:
        setPicture(ControlCondensationIndicator, ImageOrangeCircle);
		SystemFunctions::progmemToBuffer(TextWatch, buffer, sizeof(buffer));
        break;
    case CondensationRisk::High:
        setPicture(ControlCondensationIndicator, ImageRedCircle);
        SystemFunctions::progmemToBuffer(TextHighRisk, buffer, sizeof(buffer));
        break;
	}

    sendText(ControlCondensation, buffer);
}

void PageEnvironment::updateDewPoint()
{
    if (isnan(_lastTemp) || isnan(_lastHumidity))
    {
        sendText(ControlDewPoint, NoValueText);
        setPicture(ControlDewPointIndicator, ImageQuestionMark);
        return;
    }

    double dewPoint = Environment::dewPoint(_lastTemp, _lastHumidity);

    char cmd[10];
    dtostrf(dewPoint, 4, 1, cmd);

    size_t len = strlen(cmd);

    // Append degree symbol (character 176) and "C"
    if (len + 2 < sizeof(cmd))
    {
        cmd[len] = (char)176;
        cmd[len + 1] = '\0';
        strcat(cmd, CelsiusSuffix);
    }

	sendText(ControlDewPoint, cmd);

    updateComfortDescription(dewPoint);
}

void PageEnvironment::updateComfortDescription(double dewPoint)
{
    char buffer[24];

    if (_lastTemp < 10 && _lastHumidity > 70)
    {
		SystemFunctions::progmemToBuffer(TextColdDamp, buffer, sizeof(buffer));
    }
    else if (_lastTemp < 12)
    {
		SystemFunctions::progmemToBuffer(TextCold, buffer, sizeof(buffer));
    }
    else if (_lastTemp > 26 && _lastHumidity > 65)
    {
		SystemFunctions::progmemToBuffer(TextWarmSticky, buffer, sizeof(buffer));
    }
    else if (dewPoint > 18)
    {
		SystemFunctions::progmemToBuffer(TextHumid, buffer, sizeof(buffer));
    }
    else if (_lastTemp >= 18 && _lastTemp <= 24 && _lastHumidity < 65)
    {
		SystemFunctions::progmemToBuffer(TextComfortable, buffer, sizeof(buffer));
    }
    else
    {
		SystemFunctions::progmemToBuffer(TextFair, buffer, sizeof(buffer));
    }

    sendText(ControlComfort, buffer);
}

#endif // NEXTION_DISPLAY_DEVICE
