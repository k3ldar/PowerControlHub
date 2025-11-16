#pragma once

#include <Arduino.h>
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#include "ConfigManager.h"

class SensorCommandHandler : public BaseBoatCommandHandler
{
public:
    // Constructor: pass the NextionControl pointer so we can notify the current page
    explicit SensorCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager);

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
    const String* supportedCommands(size_t& count) const override;
};