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

#if defined(BOAT_CONTROL_PANEL)
#include <NextionControl.h>
#include "BoatControlPanelConstants.h"
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#else
#include "SharedBaseCommandHandler.h"
#endif

#if defined(FUSE_BOX_CONTROLLER)
class ConfigSyncManager; // Forward declaration
class ConfigController;  // Forward declaration
#endif


// Define the base class type based on the build configuration
#if defined(BOAT_CONTROL_PANEL)
#define SENSOR_BASE_CLASS BaseBoatCommandHandler
#elif defined(FUSE_BOX_CONTROLLER)
#define SENSOR_BASE_CLASS SharedBaseCommandHandler
#else
#error "Either BOAT_CONTROL_PANEL or FUSE_BOX_CONTROLLER must be defined"
#endif

class AckCommandHandler : public SENSOR_BASE_CLASS
{
private:
#if defined(BOAT_CONTROL_PANEL)
    bool processHeartbeatAck(SerialCommandManager* sender, const char* key, const char* value);
    bool processWarningsListAck(SerialCommandManager* sender, const char* key, const char* value, const StringKeyValue params[], uint8_t paramCount);
#elif defined(FUSE_BOX_CONTROLLER)
    bool processConfigAck(SerialCommandManager* sender, const char* key, const char* value);

    ConfigSyncManager* _configSyncManager;
    ConfigController* _configController;
#endif

public:
#if defined(BOAT_CONTROL_PANEL)
    explicit AckCommandHandler(BroadcastManager* computerCommandManager
        , NextionControl* nextionControl
        , WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
    explicit AckCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);

    // Set the config sync manager (optional - needed for config sync feature)
    void setConfigSyncManager(ConfigSyncManager* syncManager, ConfigController* configController);
#endif


    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

};

#undef SENSOR_BASE_CLASS