#pragma once

#include <Arduino.h>
#include "Config.h"
#include "ConfigManager.h"
#include "BaseCommandHandler.h"
#include "SoundManager.h"
#include "RelayCommandHandler.h"
#include "WifiController.h"

class BluetoothController;

class ConfigCommandHandler : public BaseCommandHandler
{
private:
    SoundManager* _soundManager;
    BluetoothController* _bluetoothController;
	WifiController* _wifiController;
	RelayCommandHandler* _relayCommandHandler;

    void updateSoundManagerConfig(Config* config);
public:
    explicit ConfigCommandHandler(SoundManager* soundManager, BluetoothController* bluetoothController, 
		WifiController* wifiController, RelayCommandHandler* relayCommandHandler);

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount) override;
    const String* supportedCommands(size_t& count) const override;
};