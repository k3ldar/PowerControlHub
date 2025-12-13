#pragma once

#include <Arduino.h>
#include "ConfigManager.h"
#include "Local.h"
#include "SystemFunctions.h"

#ifdef FUSE_BOX_CONTROLLER
#include "SharedBaseCommandHandler.h"
#endif

#ifdef BOAT_CONTROL_PANEL
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
