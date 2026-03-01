#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "WifiController.h"

SystemCommandHandler::SystemCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcaster, warningManager)
{
}

SystemCommandHandler::~SystemCommandHandler()
{
}

const char* const* SystemCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { 
        SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage,
        SystemBluetoothStatus, SystemWifiStatus, SystemSetDateTime, SystemGetDateTime,
        SystemSdCardPresent, SystemSdCardLogFileSize, SystemRtcDiagnostic, SystemUptime
    };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)params;
    (void)paramCount;

    if (strcmp(command, SystemHeartbeatCommand) == 0)
    {
        for (uint8_t i = 0; i < paramCount; i++)
        {
            // Handle time synchronization parameter (t=timestamp)
            if (strcmp(params[i].key, "t") == 0)
            {
                unsigned long timestamp = static_cast<unsigned long>(strtoul(params[i].value, nullptr, 0));
                if (timestamp > 0)
                {
                    DateTimeManager::setDateTime(timestamp);
                }
            }
            // Handle warnings parameter (w=0x00) - received from Control Panel
            else if (strcmp(params[i].key, "w") == 0)
            {
                // Warning bitmask received from Control Panel
                // Could be processed here if needed for logging or monitoring
                // Currently just acknowledged
            }
        }

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

#if defined(BLUETOOTH_SUPPORT)
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
#endif

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
    else if (strcmp(command, SystemSetDateTime) == 0 && paramCount == 1)
    {
        bool success = false;

        // Only supports Unix timestamp (all digits)
        unsigned long timestamp = static_cast<unsigned long>(strtoul(params[0].value, nullptr, 0));
        if (timestamp > 0)
        {
            DateTimeManager::setDateTime(timestamp);
            success = true;
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
    else if (strcmp(command, SystemSdCardPresent) == 0)
    {
        bool present = false;

#if defined(SD_CARD_SUPPORT)
        // Check SD card presence via MicroSdDriver
        MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
        present = sdDriver.isCardPresent();
#endif

        char value = present ? '1' : '0';
        StringKeyValue param = makeParam(ValueParamName, value);
        sendAckOk(sender, command, &param);
    }
    else if (strcmp(command, SystemSdCardLogFileSize) == 0)
    {
        uint32_t fileSize = 0;

#if defined(SD_CARD_SUPPORT)
        if (_sdCardLogger)
        {
            fileSize = _sdCardLogger->getCurrentLogFileSize();
        }
#endif

        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        snprintf_P(param.value, sizeof(param.value), PSTR("%lu"), (unsigned long)fileSize);
        sendAckOk(sender, command, &param);
    }
    else if (strcmp(command, SystemRtcDiagnostic) == 0)
    {
#if defined(BOAT_CONTROL_PANEL)
        char diagnosticMsg[64];

        bool success = DateTimeManager::rtcDiagnostic(diagnosticMsg, sizeof(diagnosticMsg));
        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        strncpy(param.value, diagnosticMsg, sizeof(param.value));

        if (success)
        {
            sendAckOk(sender, command, &param);
        }
        else
        {
            sendAckErr(sender, command, param.value);
        }
#endif
    }
    else if (strcmp(command, SystemUptime) == 0)
    {
        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        TimeParts tp = SystemFunctions::msToTimeParts(SystemFunctions::millis64());
        SystemFunctions::formatTimeParts(param.value, sizeof(param.value), tp);
        sendAckOk(sender, command, &param);
        }
    else
    {
        sendAckErr(sender, command, F("Unknown system command"));
    }

    return true;
}

#if defined(SD_CARD_SUPPORT)
void SystemCommandHandler::setSdCardLogger(SdCardLogger* sdCardLogger)
{
    _sdCardLogger = sdCardLogger;
}
#endif

void SystemCommandHandler::setWifiController(WifiController* wifiController)
{ 
    _wifiController = wifiController;
}

#if defined(MQTT_SUPPORT)
void SystemCommandHandler::setMqttController(MQTTController* mqttController)
{
    _mqttController = mqttController;
}
#endif