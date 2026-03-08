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
#include <Arduino.h>
#include "BaseBoatPage.h"

BaseBoatPage::BaseBoatPage(Stream* serialPort, 
                           WarningManager* warningMgr,
                           SerialCommandManager* commandMgrLink,
                           SerialCommandManager* commandMgrComputer) 
    : BaseDisplayPage(serialPort), 
      _config(nullptr),
      _commandMgrLink(commandMgrLink),
      _commandMgrComputer(commandMgrComputer),
      _warningManager(warningMgr)
{
}

BaseBoatPage::~BaseBoatPage()
{
    // Base destructor - no cleanup needed currently
}

void BaseBoatPage::configSet(Config* config)
{
    _config = config;
}

void BaseBoatPage::configUpdated()
{
    // Default implementation - override in derived classes if needed
}

uint8_t BaseBoatPage::getButtonColor(uint8_t buttonIndex, bool isOn, uint8_t maxButtons)
{
    Config* config = getConfig();
    if (!config || buttonIndex >= maxButtons)
    {
        // Default: grey off, blue on
        return isOn ? ImageButtonColorBlue : ImageButtonColorGrey;
    }

    if (isOn)
    {
        // Check if a custom color is configured for this button
        uint8_t configuredColor = config->buttonImage[buttonIndex];
        if (configuredColor != DefaultValue &&
            configuredColor >= ImageButtonColorBlue &&
            configuredColor <= ImageButtonColorYellow)
        {
            return configuredColor;
        }

        // Default ON color is blue
        return ImageButtonColorBlue;
    }
    else
    {
        // OFF state always uses grey
        return ImageButtonColorGrey;
    }
}