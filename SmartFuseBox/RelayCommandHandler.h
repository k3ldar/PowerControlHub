#pragma once
#include "BaseCommandHandler.h"
#include "ConfigManager.h"
#include "RelayController.h"


// internal message handlers
class RelayCommandHandler : public BaseCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;
    RelayController* _relayController;
    void broadcastRelayStatus(const String& cmd, const StringKeyValue* param = nullptr);

public:
    RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, RelayController* relayController);
    ~RelayCommandHandler();
    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;

    const char* const* supportedCommands(size_t& count) const override;

    void setup();
    void configUpdated(Config* config);

    uint8_t getRelayCount() const { return _relayController ? _relayController->getRelayCount() : 0; }

};

