#pragma once

#include <Arduino.h>
#include "Config.h"
#include "ConfigManager.h"
#include "BaseCommandHandler.h"
#include "SoundManager.h"

class BluetoothController;

class ConfigCommandHandler : public BaseCommandHandler
{
private:
    SoundManager* _soundManager;
    BluetoothController* _bluetoothController;

    void updateSoundManagerConfig(Config* config);
public:
    explicit ConfigCommandHandler(SoundManager* soundManager, BluetoothController* bluetoothController);

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
    const String* supportedCommands(size_t& count) const override;
};