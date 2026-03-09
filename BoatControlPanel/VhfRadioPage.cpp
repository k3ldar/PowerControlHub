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
#include "VhfRadioPage.h"

#include "Config.h"
#include "Phonetic.h"
#include "SystemFunctions.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonDistress = 8;
constexpr uint8_t ButtonChannels = 9;

constexpr char ControlBoatName[] = "tVhfBoatName";
constexpr char ControlHomePort[] = "tVhfHomePort";
constexpr char ControlMmsi[] = "tVhfMMSI";
constexpr char ControlCallSign[] = "tVhfCallSign";

static const char BoatNameFmt[] PROGMEM = "Boat Name: %s";
static const char HomePortFmt[] PROGMEM = "Home Port: %s";
static const char MmsiFmt[] PROGMEM = "MMSI: %s";
static const char CallSignFmt[] PROGMEM = "Call Sign: %s\r\n%s";


VhfRadioPage::VhfRadioPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void VhfRadioPage::onEnterPage()
{
	Config* config = getConfig();

    if (!config)
        return;

    // Called when page becomes active
    char buffer[50];
    snprintf_P(buffer, sizeof(buffer), BoatNameFmt, config->name);
    sendText(ControlBoatName, buffer);
	snprintf_P(buffer, sizeof(buffer), HomePortFmt, config->homePort);
	sendText(ControlHomePort, buffer);
    snprintf_P(buffer, sizeof(buffer), MmsiFmt, config->mMSI);
	sendText(ControlMmsi, buffer);

    char phoneticBuf[128];
	phoneticize(config->callSign, phoneticBuf, sizeof(phoneticBuf), ", ");

    char callSignBuf[128];
    snprintf_P(callSignBuf, sizeof(callSignBuf), CallSignFmt, config->callSign, phoneticBuf);

	char wrappedBuffer[134];
    SystemFunctions::wrapTextAtWordBoundary(callSignBuf, wrappedBuffer, sizeof(wrappedBuffer), 30);

    sendText(ControlCallSign, callSignBuf);
}

void VhfRadioPage::begin()
{
    // not used just now
}

void VhfRadioPage::refresh(unsigned long now)
{
    (void)now;
	//nothing to refresh currently
}

// Handle touch events for buttons
void VhfRadioPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        setPage(PageRelay);
        return;
    case ButtonNext:
        setPage(PageSoundSignals);
        return;
	case ButtonDistress:
		setPage(PageVhfDistress);
        return;
    case ButtonChannels:
        setPage(PageVhfChannels);
		return;
    }
}
