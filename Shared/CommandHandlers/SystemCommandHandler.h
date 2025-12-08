#pragma once

#include "SharedBaseCommandHandler.h"
#include "BroadcastManager.h"
#include "SystemDefinitions.h"
#include "SharedFunctions.h"
#include "WarningManager.h"
#include "Local.h"

#if defined(ARDUINO_UNO_R4)
#include "WifiController.h"
#endif

class SystemCommandHandler : public SharedBaseCommandHandler
{
public:
    explicit SystemCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager);
    ~SystemCommandHandler();

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount) override;
    const String* supportedCommands(size_t& count) const override;

#if defined(ARDUINO_UNO_R4)
    // Add this method
    void setWifiController(WifiController* wifiController);
#endif

private:
#if defined(ARDUINO_UNO_R4)
    WifiController* _wifiController = nullptr;
#endif
};
