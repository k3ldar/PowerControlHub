/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#if defined(WIFI_SUPPORT)
#include "WifiController.h"
#endif
#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
#include "FirmwareVersion.h"
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
    static const char* cmds[] = { 
        SystemHeartbeatCommand, SystemInitialized, SystemFreeMemory, SystemCpuUsage,
        SystemBluetoothStatus, SystemWifiStatus, SystemSetDateTime, SystemGetDateTime,
        SystemSdCardPresent, SystemSdCardLogFileSize, SystemRtcDiagnostic, SystemUptime,
        SystemCheckForUpdate, SystemOtaStatus
    };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SystemCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)params;
    (void)paramCount;

    if (SystemFunctions::commandMatches(command, SystemHeartbeatCommand))
    {
        // Use BaseConfigCommandHandler helpers to read named params
        const char* tStr = getParamValue(params, paramCount, "t");
        if (tStr)
        {
            uint64_t timestamp = static_cast<uint64_t>(strtoull(tStr, nullptr, 0));
            if (timestamp > 0)
            {
                DateTimeManager::setDateTime(timestamp);
            }
        }

        // Warnings parameter (w) is currently ignored but read here for completeness
        const char* wStr = getParamValue(params, paramCount, "w");
        (void)wStr;

        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SystemInitialized))
    {
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SystemFreeMemory))
    {
        StringKeyValue param;
		strncpy(param.key, ValueParamName, sizeof(param.key));
		snprintf_P(param.value, sizeof(param.value), PSTR("%u"), SystemFunctions::freeMemory());
        sendAckOk(sender, command, &param);
    }
	else if (SystemFunctions::commandMatches(command, SystemCpuUsage))
    {
        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        snprintf_P(param.value, sizeof(param.value), PSTR("%u"), SystemCpuMonitor::getCpuUsage());
        sendAckOk(sender, command, &param);
    }
	else if (SystemFunctions::commandMatches(command, SystemBluetoothStatus))
    {
		Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        
        if (config)
			enabled = config->network.bluetoothEnabled;
            
        if (_broadcaster)
        {
			char value = enabled ? '1' : '0';
            StringKeyValue param = makeParam(ValueParamName, value);
            sendAckOk(sender, command, &param);
        }
    }
#if defined(WIFI_SUPPORT)
    else if (SystemFunctions::commandMatches(command, SystemWifiStatus))
    {
        Config* config = ConfigManager::getConfigPtr();

        bool enabled = false;
        char ipAddress[MaxIpAddressLength] = "0.0.0.0";
		char ssid[MaxSSIDLength] = "";
        int rssi = 0;

        if (config)
            enabled = config->network.wifiEnabled;

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
    else if (SystemFunctions::commandMatches(command, SystemSetDateTime) && paramCount == 1)
    {
        bool success = false;

        // Only supports Unix timestamp (all digits)
        uint64_t timestamp = static_cast<uint64_t>(strtoull(params[0].value, nullptr, 0));
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
	else if (SystemFunctions::commandMatches(command, SystemGetDateTime))
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
    else if (SystemFunctions::commandMatches(command, SystemSdCardPresent))
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
    else if (SystemFunctions::commandMatches(command, SystemSdCardLogFileSize))
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
    else if (SystemFunctions::commandMatches(command, SystemRtcDiagnostic))
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
    else if (SystemFunctions::commandMatches(command, SystemUptime))
    {
        StringKeyValue param;
        strncpy(param.key, ValueParamName, sizeof(param.key));
        TimeParts tp = SystemFunctions::msToTimeParts(SystemFunctions::millis64());
        SystemFunctions::formatTimeParts(param.value, sizeof(param.value), tp);
        sendAckOk(sender, command, &param);
        }
#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
    else if (SystemFunctions::commandMatches(command, SystemCheckForUpdate))
    {
        if (_otaManager)
        {
            // apply=1 triggers a check and immediately applies the update if one is found.
            const char* applyStr = getParamValue(params, paramCount, "apply");
            bool applyNow = applyStr && (applyStr[0] == '1');
            _otaManager->triggerCheck(applyNow);

            // Respond with the current state — the async result will be broadcast later.
            char current[16];
            snprintf(current, sizeof(current), "v%u.%u.%u.%u",
                FirmwareMajor, FirmwareMinor, FirmwarePatch, FirmwareBuild);

            const char* stateStr = "idle";

            switch (_otaManager->getState())
            {
                case OtaState::Idle:
                    stateStr = "triggered";
                    break;
                case OtaState::Checking:
                    stateStr = "checking";
                    break;
                case OtaState::UpdateAvailable:
                    stateStr = "available";
                    break;
                case OtaState::Downloading:
                    stateStr = "downloading";
                    break;
                case OtaState::Rebooting:
                    stateStr = "rebooting";
                    break;
                case OtaState::Failed:
                    stateStr = "failed";
                    break;
                case OtaState::UpToDate:
                    stateStr = "uptodate";
                    break;
            }

            constexpr uint8_t argCount = 3;
            StringKeyValue respParams[argCount];
            strncpy(respParams[0].key, "v", sizeof(respParams[0].key));
            strncpy(respParams[0].value, current, sizeof(respParams[0].value));
            strncpy(respParams[1].key, "av", sizeof(respParams[1].key));
            strncpy(respParams[1].value, _otaManager->getAvailableVersion(), sizeof(respParams[1].value));
            strncpy(respParams[2].key, "s", sizeof(respParams[2].key));
            strncpy(respParams[2].value, stateStr, sizeof(respParams[2].value));
            sendAckOk(sender, command, respParams, argCount);
        }
        else
        {
            sendAckErr(sender, command, F("OTA not available"));
        }
    }
    else if (SystemFunctions::commandMatches(command, SystemOtaStatus))
    {
        char current[16];
        snprintf(current, sizeof(current), "v%u.%u.%u.%u",
            FirmwareMajor, FirmwareMinor, FirmwarePatch, FirmwareBuild);

        const char* stateStr   = "disabled";
        const char* avVersion  = "";
        char        autoApply  = '0';

        if (_otaManager)
        {
            switch (_otaManager->getState())
            {
                case OtaState::Idle:             stateStr = "idle";        break;
                case OtaState::Checking:         stateStr = "checking";    break;
                case OtaState::UpdateAvailable:  stateStr = "available";   break;
                case OtaState::Downloading:      stateStr = "downloading"; break;
                case OtaState::Rebooting:        stateStr = "rebooting";   break;
                case OtaState::Failed:           stateStr = "failed";      break;
                case OtaState::UpToDate:         stateStr = "uptodate";    break;
            }
            avVersion = _otaManager->getAvailableVersion();

            SystemHeader* hdr = ConfigManager::getHeaderPtr();
            if (hdr && (hdr->reserved[0] & OtaFlagAutoApply))
                autoApply = '1';
        }

        constexpr uint8_t argCount = 4;
        StringKeyValue respParams[argCount];
        strncpy(respParams[0].key, "v",  sizeof(respParams[0].key));
        strncpy(respParams[0].value, current, sizeof(respParams[0].value));
        strncpy(respParams[1].key, "av", sizeof(respParams[1].key));
        strncpy(respParams[1].value, avVersion, sizeof(respParams[1].value));
        strncpy(respParams[2].key, "s",  sizeof(respParams[2].key));
        strncpy(respParams[2].value, stateStr, sizeof(respParams[2].value));
        strncpy(respParams[3].key, "auto", sizeof(respParams[3].key));
        respParams[3].value[0] = autoApply;
        respParams[3].value[1] = '\0';
        sendAckOk(sender, command, respParams, argCount);
    }
#endif // OTA_AUTO_UPDATE
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

#if defined(WIFI_SUPPORT)
void SystemCommandHandler::setWifiController(WifiController* wifiController)
{ 
    _wifiController = wifiController;
}
#endif

#if defined(MQTT_SUPPORT)
void SystemCommandHandler::setMqttController(MQTTController* mqttController)
{
    _mqttController = mqttController;
}
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
void SystemCommandHandler::setOtaManager(OtaManager* otaManager)
{
    _otaManager = otaManager;
}
#endif