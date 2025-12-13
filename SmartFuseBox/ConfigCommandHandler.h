#pragma once

#include <Arduino.h>
#include "Config.h"
#include "ConfigManager.h"
#include "BaseCommandHandler.h"
#include "SoundController.h"
#include "RelayCommandHandler.h"
#include "WifiController.h"

class BluetoothController;

class ConfigCommandHandler : public BaseCommandHandler
{
private:
    SoundController* _soundController;
    BluetoothController* _bluetoothController;
	WifiController* _wifiController;
	RelayCommandHandler* _relayCommandHandler;

    void updateSoundControllerConfig(Config* config);
public:
    explicit ConfigCommandHandler(SoundController* soundController, BluetoothController* bluetoothController,
		WifiController* wifiController, RelayCommandHandler* relayCommandHandler);

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;
};