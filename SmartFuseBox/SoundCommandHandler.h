#pragma once

#include <Arduino.h>
#include "BaseCommandHandler.h"
#include "SoundManager.h"


// internal message handlers
class SoundCommandHandler : public BaseCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;
    SoundManager* _soundManager;

public:
    SoundCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, SoundManager* soundManager);
    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;

    const String* supportedCommands(size_t& count) const override;
private:
    void broadcast(const String& cmd, const StringKeyValue* param = nullptr);
};
