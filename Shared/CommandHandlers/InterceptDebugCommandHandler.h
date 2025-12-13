#pragma once

#include <SerialCommandManager.h>

class InterceptDebugHandler : public ISerialCommandHandler {
private:
    BroadcastManager* _broadcastManager;
public:
    InterceptDebugHandler(BroadcastManager* broadcastManager)
        : _broadcastManager(broadcastManager)
    {
    }

    bool supportsCommand(const char* command) const override
    {
		(void)command;

        // This handler intercepts all commands for debugging purposes
        return true;
    }

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override
    {
		(void)command;
		(void)params;
		(void)paramCount;
        _broadcastManager->getComputerSerial()->sendCommand(sender->getRawMessage(), "");
		return false; // Indicate that we did not fully handle the command
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        // Return empty array since we override supportsCommand() to claim all commands
        count = 0;
        return nullptr;
    }
};