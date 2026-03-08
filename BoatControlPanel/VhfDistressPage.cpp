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
#include "VhfDistressPage.h"
#include "Phonetic.h"
#include "SystemFunctions.h"

// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 2;

constexpr char ControlCallSign[] = "tMDCallSign";
constexpr char ControlPosition[] = "tMDPosition";

// Maximum safe text length for Nextion text components (conservative limit)

constexpr size_t MaxLineLength = 42;
static const char DistressCallSign[] PROGMEM = "This is %s, Call Sign %s,\r\nMMSI %s.";
static const char DistressPosition[] PROGMEM = "My position is %d degrees, %s minutes %s and %d degrees, %s minutes %s.";

VhfDistressPage::VhfDistressPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void VhfDistressPage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::GpsLatitude) && data != nullptr)
    {
        // FloatStateUpdate is used for GPS lat/lon notifications
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        _lastLatitude = update->value;
        updateDisplay();
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::GpsLongitude) && data != nullptr)
    {
        const FloatStateUpdate* update = static_cast<const FloatStateUpdate*>(data);
        _lastLongitude = update->value;
        updateDisplay();
    }
}

void VhfDistressPage::onEnterPage()
{
    updateDisplay();
}

void VhfDistressPage::begin()
{

}

void VhfDistressPage::refresh(unsigned long now)
{
    (void)now;
}

void VhfDistressPage::updateDisplay()
{
    Config* config = getConfig();

    if (!config)
    {
        return;
    }

    const char* name = (config->name && config->name[0]) ? config->name : "UNKNOWN";
    const char* callSign = (config->callSign && config->callSign[0]) ? config->callSign : "N/A";
    const char* mmsi = (config->mMSI && config->mMSI[0]) ? config->mMSI : "N/A";

    char buffer[170];  // Large enough for both operations

    char phoneticCallSign[85];
    phoneticize(callSign, phoneticCallSign, sizeof(phoneticCallSign), ", ");

    char tempBuffer[140];
    snprintf_P(tempBuffer, sizeof(tempBuffer), DistressCallSign, name, phoneticCallSign, mmsi);
    SystemFunctions::wrapTextAtWordBoundary(tempBuffer, buffer, sizeof(buffer), MaxLineLength);

    sendText(ControlCallSign, buffer);

    formatGpsPosition(_lastLatitude, _lastLongitude, buffer, sizeof(buffer));
    sendText(ControlPosition, buffer);
}

// Handle touch events for buttons
void VhfDistressPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        setPage(PageVhfRadio);
        return;
    }
}

void VhfDistressPage::formatGpsPosition(double latitude, double longitude, char* outBuf, size_t outBufSize)
{
    // Check for invalid GPS data
    if (isnan(latitude) || isnan(longitude))
    {
        snprintf_P(outBuf, outBufSize, PSTR("GPS position unavailable"));
        return;
    }

    // Calculate latitude components
    double latAbs = latitude < 0.0 ? -latitude : latitude;
    int latDeg = static_cast<int>(latAbs);
    double latMin = (latAbs - latDeg) * 60.0;
    const char* latDir = latitude >= 0.0 ? "North" : "South";

    // Calculate longitude components
    double lonAbs = longitude < 0.0 ? -longitude : longitude;
    int lonDeg = static_cast<int>(lonAbs);
    double lonMin = (lonAbs - lonDeg) * 60.0;
    const char* lonDir = longitude >= 0.0 ? "East" : "West";

    char latMinStr[8];
    char lonMinStr[8];
    dtostrf(latMin, 6, 3, latMinStr);
    dtostrf(lonMin, 6, 3, lonMinStr);

    // Build the final string using only integer and string formatting
    snprintf_P(outBuf, outBufSize,
        DistressPosition,
        latDeg, latMinStr, latDir, lonDeg, lonMinStr, lonDir);
}

