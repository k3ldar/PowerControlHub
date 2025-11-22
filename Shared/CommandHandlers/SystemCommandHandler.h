#pragma once
#include "BaseCommandHandler.h"
#include "BroadcastManager.h"
#include "SharedConstants.h"
#include "SharedFunctions.h"
#include "Local.h"

class SystemCommandHandler : public BaseCommandHandler
{
private:
    BroadcastManager* _broadcaster;
    
public:
    SystemCommandHandler(BroadcastManager* broadcaster);
    ~SystemCommandHandler();
    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;

    const String* supportedCommands(size_t& count) const override;
    
private:
    uint16_t freeRam();
};
