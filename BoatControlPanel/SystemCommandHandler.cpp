#include "SystemCommandHandler.h"

SystemCommandHandler::SystemCommandHandler(BroadcastManager* broadcaster)
    : _broadcaster(broadcaster)
{
}

SystemCommandHandler::~SystemCommandHandler()
{
}

const String* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory };
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
        StringKeyValue param = { ValueParamName, String(freeRam()) };
        sendAckOk(sender, cmd, &param);
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    return true;
}

uint16_t SystemCommandHandler::freeRam() {
    extern int __heap_start, * __brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
