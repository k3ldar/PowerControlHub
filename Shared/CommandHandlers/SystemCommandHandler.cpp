#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"

SystemCommandHandler::SystemCommandHandler(BroadcastManager* broadcaster)
    : _broadcaster(broadcaster)
{
}

SystemCommandHandler::~SystemCommandHandler()
{
}

const String* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage,
        SystemBluetoothStatus };
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
	else if (cmd == SystemBluetoothStatus)
    {
		Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        
        if (config)
			enabled = config->bluetoothEnabled;
            
        if (_broadcaster)
        {
            StringKeyValue param = { ValueParamName, enabled ? F("1") : F("0") };
            sendAckOk(sender, cmd, &param);
        }
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    return true;
}
