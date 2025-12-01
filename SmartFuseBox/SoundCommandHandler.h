#pragma once

#include <Arduino.h>
#include "BaseCommandHandler.h"
#include "SoundController.h"


// internal message handlers
class SoundCommandHandler : public BaseCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;
    SoundController* _soundController;

public:
    SoundCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, SoundController* soundController);
    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount) override;

    const String* supportedCommands(size_t& count) const override;
private:
    void broadcast(const String& cmd, const StringKeyValue* param = nullptr);
};
