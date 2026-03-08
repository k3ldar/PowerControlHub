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
#pragma once
#include <Arduino.h>
#include "Config.h"
#include "ConfigManager.h"
#include "HomePage.h"
#include "BaseCommandHandler.h"
#include "BroadcastManager.h"

class ConfigCommandHandler : public BaseCommandHandler
{
public:
    // Constructor: pass the HomePage pointer so we can notify UI when saved/updated
    explicit ConfigCommandHandler(BroadcastManager* broadcastManager, HomePage* homePage);

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

private:
    BroadcastManager* _broadcastManager;
    HomePage* _homePage;
};