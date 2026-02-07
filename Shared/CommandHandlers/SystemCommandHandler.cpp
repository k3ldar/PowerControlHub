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
		strncpy(param.key, ValueParamName, sizeof(param.key));
		snprintf_P(param.value, sizeof(param.value), PSTR("%u"), SystemFunctions::freeMemory());
        sendAckOk(sender, command, &param);
    }
	else if (strcmp(command, SystemCpuUsage) == 0)
    {
        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        snprintf_P(param.value, sizeof(param.value), PSTR("%u"), SystemCpuMonitor::getCpuUsage());
        sendAckOk(sender, command, &param);
    }

#if defined(ARDUINO_UNO_R4)
	else if (strcmp(command, SystemBluetoothStatus) == 0)
    {
		Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        
        if (config)
			enabled = config->bluetoothEnabled;
            
        if (_broadcaster)
        {
			char value = enabled ? '1' : '0';
            StringKeyValue param = makeParam(ValueParamName, value);
            sendAckOk(sender, command, &param);
        }
    }
    else if (strcmp(command, SystemWifiStatus) == 0)
    {
        Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        char ipAddress[MaxIpAddressLength] = "0.0.0.0";
		char ssid[MaxSSIDLength] = "";
        int rssi = 0;

        if (config)
            enabled = config->wifiEnabled;

        // Get IP address from WifiController if available
        if (_wifiController && enabled && _wifiController->isEnabled())
        {

            _wifiController->getServer()->getIpAddress(ipAddress, MaxIpAddressLength);
			_wifiController->getServer()->getSSID(ssid, MaxSSIDLength);
			rssi = _wifiController->getServer()->getSignalStrength();
        }

        if (_broadcaster)
        {
            constexpr uint8_t argCount = 4;
			char enabledValue = enabled ? '1' : '0';
            StringKeyValue params[argCount] = {
                makeParam(ValueParamName, enabledValue),
                makeParam("ip", ipAddress),
				makeParam("ssid", ssid),
                makeParam("rssi", rssi)
            };
            sendAckOk(sender, command, params, argCount);
        }
    }
#endif
    else if (strcmp(command, SystemSetDateTime) == 0 && paramCount == 1)
    {
        bool success = false;
        _broadcaster->getComputerSerial()->sendDebug(command, params[0].value);
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
			strncpy(param.key, ValueParamName, sizeof(param.key));
            DateTimeManager::formatDateTime(param.value, sizeof(param.value));
            sendAckOk(sender, command, &param);
        }
        else
        {
            _broadcaster->getComputerSerial()->sendDebug(command, F("Invalid Datetime"));
            sendAckErr(sender, command, F("Invalid datetime format"));
        }
        
        return true;
    }
	else if (strcmp(command, SystemGetDateTime) == 0)
    {
        if (DateTimeManager::isTimeSet())
        {
            StringKeyValue param;
            strncpy(param.key, ValueParamName, sizeof(param.key));
            DateTimeManager::formatDateTime(param.value, sizeof(param.value));
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