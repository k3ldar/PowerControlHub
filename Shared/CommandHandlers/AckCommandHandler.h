#pragma once

#include <Arduino.h>

#include "Local.h"
#include "ConfigManager.h"

#ifdef BOAT_CONTROL_PANEL
#include <NextionControl.h>
#include "BoatControlPanelConstants.h"
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#else
#include "SharedBaseCommandHandler.h"
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
public:
#if defined(BOAT_CONTROL_PANEL)
    explicit AckCommandHandler(BroadcastManager* computerCommandManager
        , NextionControl* nextionControl
        , WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
    explicit AckCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);
#endif
    

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

private:
#ifdef BOAT_CONTROL_PANEL
    bool processHeartbeatAck(SerialCommandManager* sender, const String& key, const String& value);
    bool processWarningsListAck(SerialCommandManager* sender, const String& key, const String& value, const StringKeyValue params[], uint8_t paramCount);
#endif
};

#undef SENSOR_BASE_CLASS