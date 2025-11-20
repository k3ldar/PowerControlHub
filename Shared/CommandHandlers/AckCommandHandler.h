#pragma once
#include <Arduino.h>

#include "Local.h"
#include "ConfigManager.h"
#ifdef BOAT_CONTROL_PANEL
#include "BoatControlPanelConstants.h"
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#else
#include "SharedBaseCommandHandler.h"
#endif

class AckCommandHandler : public
#ifdef BOAT_CONTROL_PANEL
    BaseBoatCommandHandler
#else
    SharedBaseCommandHandler
#endif
{
public:
    // Constructor: pass the NextionControl pointer so we can notify the current page
    explicit AckCommandHandler(BroadcastManager* computerCommandManager
#ifdef BOAT_CONTROL_PANEL
        , NextionControl* nextionControl, WarningManager* warningManager
#endif
    );

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
    const String* supportedCommands(size_t& count) const override;

private:
#ifdef BOAT_CONTROL_PANEL
    bool processHeartbeatAck(SerialCommandManager* sender, const String& key, const String& value);
#endif
};