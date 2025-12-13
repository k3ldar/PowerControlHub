#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"

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

const char* const* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage,
        SystemBluetoothStatus, SystemWifiStatus, SystemSetDateTime, SystemGetDateTime };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)params;
    (void)paramCount;

    if (strcmp(command, SystemHeartbeatCommand) == 0)
    {
        sendAckOk(sender, command);
    }
    else if (strcmp(command, SystemInitialized) == 0)
    {
        sendAckOk(sender, command);
    }
    else if (strcmp(command, SystemFreeMemory) == 0)
    {
        StringKeyValue param;
		strcpy(param.key, ValueParamName);
		snprintf(param.value, sizeof(param.value), "%u", SystemFunctions::freeMemory());
        sendAckOk(sender, command, &param);
    }
	else if (strcmp(command, SystemCpuUsage) == 0)
    {
        StringKeyValue param;
        strcpy(param.key, ValueParamName);
        snprintf(param.value, sizeof(param.value), "%u", SystemCpuMonitor::getCpuUsage());
        sendAckOk(sender, command, &param);
    }

#if defined(ARDUINO_UNO_R4)
	else if (strcmp(cmd, SystemBluetoothStatus) == 0)
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
    else if (strcmp(cmd, SystemWifiStatus) == 0)
    {
        Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        String ipAddress = F("0.0.0.0");
		String ssid = "";
        int rssi = 0;

        if (config)
            enabled = config->wifiEnabled;

        // Get IP address from WifiController if available
        if (_wifiController && enabled && _wifiController->isEnabled())
        {
            ipAddress = _wifiController->getServer()->getIpAddress();
			ssid = _wifiController->getServer()->getSSID();
			rssi = _wifiController->getServer()->getSignalStrength();
        }

        if (_broadcaster)
        {
            constexpr uint8_t argCount = 4;
            StringKeyValue params[argCount] = {
                { ValueParamName, enabled ? F("1") : F("0") },
                { "ip", ipAddress },
				{ "ssid", ssid },
                { "rssi", String(rssi)}
            };
            sendAckOk(sender, cmd, params, argCount);
        }
    }
#endif
    else if (strcmp(command, SystemSetDateTime) == 0 && paramCount == 1)
    {
        bool success = false;
        
        // Try ISO 8601 format first (contains 'T' or '-')
        if (strchr(params[0].value, 'T') != nullptr || strchr(params[0].value, '-') != nullptr)
        {
            success = DateTimeManager::setDateTimeISO(params[0].value);
        }
        else
        {
            // Try Unix timestamp (all digits)
            unsigned long timestamp = static_cast<unsigned long>(strtoul(params[0].value, nullptr, 0));
            if (timestamp > 0) {
                DateTimeManager::setDateTime(timestamp);
                success = true;
            }
        }
        
        if (success)
        {
            StringKeyValue param;
			strcpy(param.key, ValueParamName);
			strcpy(param.value, DateTimeManager::formatDateTime().c_str());
            sendAckOk(sender, command, &param);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid datetime format"));
        }
        
        return true;
    }
	else if (strcmp(command, SystemGetDateTime) == 0)
    {
        if (DateTimeManager::isTimeSet())
        {
            StringKeyValue param;
            strcpy(param.key, ValueParamName);
            strcpy(param.value, DateTimeManager::formatDateTime().c_str());
            sendAckOk(sender, command, &param);
        }
        else
        {
            sendAckErr(sender, command, F("Date/time not set"));
        }
        
        return true;
    }
    else
    {
        sendAckErr(sender, command, F("Unknown system command"));
    }

    return true;
}

#if defined(ARDUINO_UNO_R4)
void SystemCommandHandler::setWifiController(WifiController* wifiController)
{ 
    _wifiController = wifiController;
}
#endif