#pragma once

#include <Arduino.h>
#include "BaseCommandHandler.h"
#include "ConfigController.h"
#include "WifiController.h"

class BluetoothController;

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	WifiController* _wifiController;
    ConfigController* _configController;
public:
    explicit ConfigCommandHandler(WifiController* wifiController, ConfigController* configController);

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;
};