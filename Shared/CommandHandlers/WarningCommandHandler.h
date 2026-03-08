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

#include "Local.h"
#include "ConfigManager.h"
#include "SystemFunctions.h"

#if defined(FUSE_BOX_CONTROLLER)
#include "SharedBaseCommandHandler.h"
#endif

#if defined(BOAT_CONTROL_PANEL)
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#endif

// Define the base class type based on the build configuration
#if defined(BOAT_CONTROL_PANEL)
    #define WARNING_BASE_CLASS BaseBoatCommandHandler
#elif defined(FUSE_BOX_CONTROLLER)
    #define WARNING_BASE_CLASS SharedBaseCommandHandler
#else
    #error "Either BOAT_CONTROL_PANEL or FUSE_BOX_CONTROLLER must be defined"
#endif

class WarningCommandHandler : public WARNING_BASE_CLASS
{
private:
	bool convertWarningTypeFromString(const char* str, WarningType& outType);
public:
#if defined(BOAT_CONTROL_PANEL)
    // Constructor: pass the NextionControl pointer so we can notify the current page
    explicit WarningCommandHandler(BroadcastManager* broadcastManager,
        NextionControl* nextionControl, WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
	explicit WarningCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);
#endif

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;
};

#undef WARNING_BASE_CLASS
