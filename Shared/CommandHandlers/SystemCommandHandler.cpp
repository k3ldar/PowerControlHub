#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"

SystemCommandHandler::SystemCommandHandler(BroadcastManager* broadcaster)
    : _broadcaster(broadcaster)
{
}

SystemCommandHandler::~SystemCommandHandler()
{
}

const String* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    (void)params;
    (void)paramCount;

    String cmd = command;
    cmd.trim();

    if (cmd == SystemHeartbeatCommand)
    {
        sendAckOk(sender, cmd);
    }
    else if (cmd == SystemInitialized)
    {
        sendAckOk(sender, cmd);
    }
    else if (cmd == SystemFreeMemory)
    {
        StringKeyValue param = { ValueParamName, String(SharedFunctions::freeMemory()) };
        sendAckOk(sender, cmd, &param);
    }
	else if (cmd == SystemCpuUsage)
    {
        StringKeyValue param = { ValueParamName, String(SystemCpuMonitor::getCpuUsage()) };
        sendAckOk(sender, cmd, &param);
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    return true;
}
