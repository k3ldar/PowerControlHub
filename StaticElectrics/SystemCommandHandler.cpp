
#include "SystemCommandHandler.h"

SystemCommandHandler::SystemCommandHandler(SerialCommandManager* commandMgrComputer)
    : _commandMgrComputer(commandMgrComputer)
{
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
	(void)params;

    // Access the in-memory config
    Config* config = ConfigManager::getConfigPtr();

    if (!config)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    if (paramCount > 0)
    {
        // This command does not accept parameters
        sendAckErr(sender, command, F("Parameters not supported"));
        return true;
	}

    // Normalize command
    String cmd = command;
    cmd.trim();

    if (cmd == SystemHeartbeatCommand)
    {
        sendAckOk(sender, cmd);
    }
    else if (cmd == SystemInitializedCommand)
    {
        sendAckOk(sender, cmd);
    }
    else if (cmd == SystemFreeMemoryCommand)
    {
        sendAckOk(sender, cmd);
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    return true;
}

const String* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SystemHeartbeatCommand, SystemInitializedCommand, SystemFreeMemoryCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}
