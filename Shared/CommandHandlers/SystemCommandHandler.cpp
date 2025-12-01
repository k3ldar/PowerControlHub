#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"

#if defined(ARDUINO_UNO_R4)
#include "WifiController.h"
#endif

SystemCommandHandler::SystemCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcaster, warningManager)
{
}

SystemCommandHandler::~SystemCommandHandler()
{
}

const String* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage,
        SystemBluetoothStatus, SystemWifiStatus };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
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

#if defined(ARDUINO_UNO_R4)
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
    else if (cmd == SystemWifiStatus)
    {
        Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        String ipAddress = F("0.0.0.0");
		String ssid = "";

        if (config)
            enabled = config->wifiEnabled;

        // Get IP address from WifiController if available
        if (_wifiController && enabled && _wifiController->isEnabled())
        {
            ipAddress = _wifiController->getServer()->getIpAddress();
			ssid = _wifiController->getServer()->getSSID();
        }

        if (_broadcaster)
        {
            constexpr uint8_t argCount = 3;
            StringKeyValue params[argCount] = {
                { ValueParamName, enabled ? F("1") : F("0") },
                { "ip", ipAddress },
				{ "ssid", ssid }
            };
            sendAckOk(sender, cmd, params, argCount);
        }
    }
#endif
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    return true;
}

#if defined(ARDUINO_UNO_R4)
void SystemCommandHandler::setWifiController(WifiController* wifiController)
{ 
    _wifiController = wifiController;
}
#endif