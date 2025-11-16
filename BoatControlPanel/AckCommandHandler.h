#pragma once
#include <Arduino.h>
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#include "ConfigManager.h"
#include "BoatControlPanelConstants.h"

class AckCommandHandler : public BaseBoatCommandHandler
{
public:
    // Constructor: pass the NextionControl pointer so we can notify the current page
    explicit AckCommandHandler(BroadcastManager* computerCommandManager, NextionControl* nextionControl, WarningManager* warningManager);

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
    const String* supportedCommands(size_t& count) const override;

private:
    // Parameter processing helpers
    bool processHeartbeatAck(SerialCommandManager* sender, const String& key, const String& value);
};