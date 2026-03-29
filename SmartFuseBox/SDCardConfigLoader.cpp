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
#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include "SDCardConfigLoader.h"
#include "ConfigManager.h"
#include "SystemFunctions.h"
#include <string.h>

SdCardConfigLoader::SdCardConfigLoader(SerialCommandManager* computerSerial,
                                       SerialCommandManager* linkSerial,
                                       ConfigController* configController,
                                       RelayController* relayController,
                                       ConfigSyncManager* configSyncManager)
    : _computerSerial(computerSerial),
      _linkSerial(linkSerial),
      _configController(configController),
      _relayController(relayController),
      _configSyncManager(configSyncManager),
      _sdConfigPresent(false)
{
}

bool SdCardConfigLoader::checkSdCard()
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
    return sdDriver.getInitState() == MicroSdInitState::Initialized && sdDriver.isCardPresent(true);
}

bool SdCardConfigLoader::configFileExists()
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
    return sdDriver.fileExists(SD_CONFIG_FILENAME);
}

bool SdCardConfigLoader::applyConfigCommand(const char* line)
{
    // Skip empty lines and comments
    if (line == nullptr || line[0] == '\0' || line[0] == '#' || line[0] == '\r' || line[0] == '\n')
    {
        return true;
    }

    // Create a mutable copy for parsing
    char buffer[SD_CONFIG_MAX_LINE_LENGTH];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Trim whitespace and newlines
    char* end = buffer + strlen(buffer) - 1;
    while (end >= buffer && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t'))
    {
        *end = '\0';
        end--;
    }

    if (buffer[0] == '\0')
    {
        return true;
    }

    // Parse command using SerialCommandManager's parser
    // Format: "CMD:param1=value1;param2=value2"
    char* colonPos = strchr(buffer, ':');
    char command[8] = {0};
    
    if (colonPos != nullptr)
    {
        size_t cmdLen = colonPos - buffer;
        if (cmdLen >= sizeof(command))
        {
            logError("Command too long", line);
            return false;
        }
        strncpy(command, buffer, cmdLen);
        command[cmdLen] = '\0';
    }
    else
    {
        // No colon means command with no params (like C0, C1, C2)
        strncpy(command, buffer, sizeof(command) - 1);
    }

    // Parse parameters manually since we're simulating command input
    const char* paramsStr = colonPos ? (colonPos + 1) : "";
    
    // Use the computer serial's command parser to validate and route the command
    // We'll construct a synthetic command and feed it through the handler chain
    
    // Parse parameters into key-value pairs
    StringKeyValue params[10];
    uint8_t paramCount = 0;
    
    if (paramsStr && *paramsStr)
    {
        char paramBuffer[SD_CONFIG_MAX_LINE_LENGTH];
        strncpy(paramBuffer, paramsStr, sizeof(paramBuffer) - 1);
        paramBuffer[sizeof(paramBuffer) - 1] = '\0';
        
        // Parse semicolon-separated parameters
        char* savePtr1 = nullptr;
        char* param = strtok_r(paramBuffer, ";", &savePtr1);
        
        while (param && paramCount < 10)
        {
            // Each param can be "key=value" or just "value"
            char* equalPos = strchr(param, '=');
            if (equalPos)
            {
                *equalPos = '\0';
                strncpy(params[paramCount].key, param, sizeof(params[paramCount].key) - 1);
                params[paramCount].key[sizeof(params[paramCount].key) - 1] = '\0';
                strncpy(params[paramCount].value, equalPos + 1, sizeof(params[paramCount].value) - 1);
                params[paramCount].value[sizeof(params[paramCount].value) - 1] = '\0';
            }
            else
            {
                // For commands like C13:SSID, the whole thing after : is the value
                // with an implied key (often "v" or empty)
                params[paramCount].key[0] = '\0';
                strncpy(params[paramCount].value, param, sizeof(params[paramCount].value) - 1);
                params[paramCount].value[sizeof(params[paramCount].value) - 1] = '\0';
            }
            paramCount++;
            param = strtok_r(nullptr, ";", &savePtr1);
        }
    }

    // Route command through the appropriate controller, using commandMatches for
    // consistent comparison (guards against prefix ambiguity e.g. R1 vs R10/R11).

    // ── Relay config commands (R6–R11) ───────────────────────────────────────
    if (SystemFunctions::commandMatches(command, RelayRename) && paramCount >= 1)
    {
        uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        int pipeIdx = SystemFunctions::indexOf(params[0].value, '|', 0);
        char shortName[ConfigShortRelayNameLength] = "";
        char longName[ConfigLongRelayNameLength] = "";

        if (pipeIdx >= 0)
        {
            SystemFunctions::substr(shortName, sizeof(shortName), params[0].value, 0, pipeIdx);
            SystemFunctions::substr(longName, sizeof(longName), params[0].value, pipeIdx + 1);
        }
        else
        {
            strncpy(shortName, params[0].value, sizeof(shortName) - 1);
        }

        RelayResult rr = _relayController->renameRelay(idx, shortName, longName);
        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }
    else if (SystemFunctions::commandMatches(command, RelaySetButtonColor) && paramCount >= 1)
    {
        uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t color = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

        RelayResult rr = _relayController->setButtonColor(relayIndex, color);
        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }
    else if (SystemFunctions::commandMatches(command, RelaySetDefaultState) && paramCount >= 1)
    {
        uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        bool state = SystemFunctions::parseBooleanValue(params[0].value);

        RelayResult rr = _relayController->setRelayDefaultState(relayIndex, state);
        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }
    else if (SystemFunctions::commandMatches(command, RelayLink) && paramCount >= 1)
    {
        uint8_t relay1 = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t relay2 = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

        RelayResult rr;
        if (relay2 == DefaultValue)
        {
            rr = _relayController->unlinkRelay(relay1);
        }
        else
        {
            rr = _relayController->linkRelays(relay1, relay2);
        }

        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }
    else if (SystemFunctions::commandMatches(command, RelaySetActionType) && paramCount >= 1)
    {
        uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t actionType = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

        RelayResult rr = _relayController->setRelayActionType(relayIndex, static_cast<RelayActionType>(actionType));
        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }
    else if (SystemFunctions::commandMatches(command, RelaySetPin) && paramCount >= 1)
    {
        uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t pin = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

        RelayResult rr = _relayController->setRelayPin(relayIndex, pin);
        if (rr != RelayResult::Success)
        {
            char errorMsg[64];
            snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(rr));
            logError(errorMsg, line);
            return false;
        }
        return true;
    }

    // ── Device / network config commands (C-series) ──────────────────────────
    ConfigResult result = ConfigResult::InvalidCommand;

    if (SystemFunctions::commandMatches(command, ConfigRename) && paramCount >= 1)
    {
        result = _configController->rename(params[0].value);
    }
    else if (SystemFunctions::commandMatches(command, ConfigMapHomeButton) && paramCount >= 1)
    {
        uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t relay  = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
        result = _configController->mapHomeButton(button, relay);
    }
    else if (SystemFunctions::commandMatches(command, ConfigBoatType) && paramCount >= 1)
    {
        result = _configController->setVesselType(static_cast<uint8_t>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigSoundStartDelay) && paramCount >= 1)
    {
        result = _configController->setsoundDelayStart(static_cast<uint16_t>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigBluetoothEnable) && paramCount >= 1)
    {
        result = _configController->setBluetoothEnabled(atoi(params[0].value) != 0);
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiEnable) && paramCount >= 1)
    {
        result = _configController->setWifiEnabled(atoi(params[0].value) != 0);
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiMode) && paramCount >= 1)
    {
        result = _configController->setWifiAccessMode(static_cast<WifiMode>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiSSID) && colonPos != nullptr)
    {
        result = _configController->setWifiSsid(colonPos + 1);
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiPassword) && colonPos != nullptr)
    {
        result = _configController->setWifiPassword(colonPos + 1);
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiPort) && paramCount >= 1)
    {
        result = _configController->setWifiPort(static_cast<uint16_t>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigWifiApIpAddress) && colonPos != nullptr)
    {
        result = _configController->setWifiIpAddress(colonPos + 1);
    }
    else if (SystemFunctions::commandMatches(command, ConfigTimeZoneOffset) && paramCount >= 1)
    {
        result = _configController->setTimezoneOffset(static_cast<int8_t>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigMmsi) && colonPos != nullptr)
    {
        result = _configController->setMmsi(colonPos + 1);
    }
    else if (SystemFunctions::commandMatches(command, ConfigCallSign))
    {
        result = _configController->setCallSign(colonPos != nullptr && *(colonPos + 1) != '\0' ? colonPos + 1 : "");
    }
    else if (SystemFunctions::commandMatches(command, ConfigHomePort))
    {
        result = _configController->setHomePort(colonPos != nullptr && *(colonPos + 1) != '\0' ? colonPos + 1 : "");
    }
    else if (SystemFunctions::commandMatches(command, ConfigLedColor) && paramCount >= 5)
    {
        uint8_t type = 0, colorSet = 0, r = 0, g = 0, b = 0;

        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "c") == 0)
                colorSet = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "r") == 0)
                r = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "g") == 0)
                g = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "b") == 0)
                b = static_cast<uint8_t>(atoi(params[i].value));
        }

        result = _configController->setLedColor(type, colorSet, r, g, b);
    }
    else if (SystemFunctions::commandMatches(command, ConfigLedBrightness) && paramCount >= 2)
    {
        uint8_t type = 0, brightness = 0;

        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "b") == 0)
                brightness = static_cast<uint8_t>(atoi(params[i].value));
        }

        result = _configController->setLedBrightness(type, brightness);
    }
    else if (SystemFunctions::commandMatches(command, ConfigLedAutoSwitch) && paramCount >= 1)
    {
        result = _configController->setLedAutoSwitch(atoi(params[0].value) != 0);
    }
    else if (SystemFunctions::commandMatches(command, ConfigLedEnable) && paramCount >= 3)
    {
        bool gps = false, warning = false, system = false;

        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "g") == 0)
                gps = (atoi(params[i].value) != 0);
            else if (strcmp(params[i].key, "w") == 0)
                warning = (atoi(params[i].value) != 0);
            else if (strcmp(params[i].key, "s") == 0)
                system = (atoi(params[i].value) != 0);
        }

        result = _configController->setLedEnableStates(gps, warning, system);
    }
    else if (SystemFunctions::commandMatches(command, ControlPanelTones) && paramCount >= 4)
    {
        uint8_t type = 0, preset = 0;
        uint16_t toneHz = 0, durationMs = 0;
        uint32_t repeatMs = 0;

        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "h") == 0)
                toneHz = static_cast<uint16_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "d") == 0)
                durationMs = static_cast<uint16_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "p") == 0)
                preset = static_cast<uint8_t>(atoi(params[i].value));
            else if (strcmp(params[i].key, "r") == 0)
                repeatMs = strtoul(params[i].value, nullptr, 0);
        }

        result = _configController->setControlPanelTones(type, preset, toneHz, durationMs, repeatMs);
    }
    else if (SystemFunctions::commandMatches(command, ConfigSdCardSpeed) && paramCount >= 1)
    {
        result = _configController->setSdCardInitializeSpeed(static_cast<uint8_t>(atoi(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, ConfigLightSensorThreshold) && paramCount >= 1)
    {
        result = _configController->setLightSensorThreshold(static_cast<uint16_t>(atoi(params[0].value)));
    }
    else
    {
        logError("Unknown or invalid command", line);
        return false;
    }

    if (result != ConfigResult::Success)
    {
        char errorMsg[64];
        snprintf(errorMsg, sizeof(errorMsg), "Command failed: %s (result=%d)", command, static_cast<uint8_t>(result));
        logError(errorMsg, line);
        return false;
    }

    return true;
}

void SdCardConfigLoader::syncConfigToLink()
{
    // Send C1 (get settings) to trigger config broadcast via LINK
    // This will make the control panel receive the new config
    if (_linkSerial)
    {
        _linkSerial->sendCommand("C1", "");
    }
}

void SdCardConfigLoader::logError(const char* message, const char* line)
{
    if (_computerSerial)
    {
        _computerSerial->sendError("SD_CFG_ERROR", message);
        if (line)
        {
            _computerSerial->sendError("SD_CFG_LINE", line);
        }
    }
}

void SdCardConfigLoader::logInfo(const char* message)
{
    if (_computerSerial)
    {
        _computerSerial->sendError("SD_CFG_INFO", message);
    }
}

bool SdCardConfigLoader::loadConfigFromSd()
{
    logInfo("Checking for SD config...");
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    // Request exclusive access (closes all open files including logger)
    if (!sdDriver.requestExclusiveAccess())
    {
        logError("Failed to get exclusive SD card access");
        return false;
    }

    if (!checkSdCard())
    {
        sdDriver.releaseExclusiveAccess();
        logInfo("SD card not present or not accessible");
        return false;
    }

    if (!configFileExists())
    {
        sdDriver.releaseExclusiveAccess();
        logInfo("Config file not found on SD card");
        return false;
    }

    logInfo("Loading config from SD card...");

    FsFile* configFile = sdDriver.openFile(MicroSdFileHandle::ConfigLoader,
        SD_CONFIG_FILENAME, O_RDONLY);

    if (!configFile)
    {
        sdDriver.releaseExclusiveAccess();
        logError("Failed to open config file");
        return false;
    }

    // Read and apply commands line by line
    char line[SD_CONFIG_MAX_LINE_LENGTH];
    uint16_t lineNumber = 0;
    uint16_t successCount = 0;
    uint16_t errorCount = 0;

    while (configFile->available())
    {
        int len = configFile->fgets(line, sizeof(line));
        lineNumber++;

        if (len > 0)
        {
            if (applyConfigCommand(line))
            {
                successCount++;
            }
            else
            {
                errorCount++;
            }
        }
    }

    sdDriver.closeFile(MicroSdFileHandle::ConfigLoader);
    sdDriver.releaseExclusiveAccess();

    // Save to EEPROM
    if (successCount > 0)
    {
        logInfo("Saving config to EEPROM...");
        ConfigResult saveResult = _configController->save();

        if (saveResult == ConfigResult::Success)
        {
            logInfo("Config saved to EEPROM");

            // Sync to control panel via LINK
            logInfo("Syncing config to control panel...");
            syncConfigToLink();

            // Disable ConfigSyncManager since SD config is authoritative
            if (_configSyncManager)
            {
                _configSyncManager->setEnabled(false);
                logInfo("ConfigSyncManager disabled (SD config active)");
            }

            _sdConfigPresent = true;

            char summary[64];
            snprintf(summary, sizeof(summary), "SD config loaded: %u commands applied, %u errors", successCount, errorCount);
            logInfo(summary);

            return true;
        }
        else
        {
            logError("Failed to save config to EEPROM");
        }
    }

    return false;
}

bool SdCardConfigLoader::reloadConfigFromSd()
{
    logInfo("Reloading config from SD card...");
    return loadConfigFromSd();
}

bool SdCardConfigLoader::exportConfigToSd()
{
    logInfo("Exporting config to SD card...");
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    // Request exclusive access (closes all open files including logger)
    if (!sdDriver.requestExclusiveAccess())
    {
        logError("Failed to get exclusive SD card access");
        return false;
    }

    if (!checkSdCard())
    {
        sdDriver.releaseExclusiveAccess();
        logError("SD card not present or not accessible");
        return false;
    }

    // Delete existing config file if present
    if (sdDriver.fileExists(SD_CONFIG_FILENAME))
    {
        sdDriver.deleteFile(SD_CONFIG_FILENAME);
    }

    FsFile* configFile = sdDriver.openFile(MicroSdFileHandle::ConfigLoader,
                                           SD_CONFIG_FILENAME, O_WRONLY | O_CREAT | O_TRUNC);
    if (!configFile)
    {
        sdDriver.releaseExclusiveAccess();
        logError("Failed to create config file");
        return false;
    }

    Config* config = _configController->getConfigPtr();
    if (!config)
    {
        sdDriver.closeFile(MicroSdFileHandle::ConfigLoader);
        sdDriver.releaseExclusiveAccess();
        logError("Config not available");
        return false;
    }

    // Write header comment
    configFile->println("# SmartFuseBox Configuration");
    configFile->println("# Generated by C30 command");
    configFile->println();

    // C3 - Boat name
    if (strlen(config->vessel.name) > 0)
    {
        configFile->print(ConfigRename);
        configFile->print(":");
        configFile->println(config->vessel.name);
    }

    // R6 - Relay names (short|long)
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print(RelayRename);
        configFile->print(":");
        configFile->print(i);
        configFile->print("=");
        configFile->print(config->relay.relays[i].shortName);
        configFile->print("|");
        configFile->println(config->relay.relays[i].longName);
    }

    // C5 - Home button mappings
    for (uint8_t i = 0; i < ConfigHomeButtons; i++)
    {
        configFile->print(ConfigMapHomeButton);
        configFile->print(":");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->relay.homePageMapping[i]);
    }

    // R7 - Button colors
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print(RelaySetButtonColor);
        configFile->print(":");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->relay.relays[i].buttonImage);
    }

    // R8 - Default relay states
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print(RelaySetDefaultState);
        configFile->print(":");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->relay.relays[i].defaultState ? "1" : "0");
    }

    // R9 - Linked relays
    for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
    {
        if (config->relay.linkedRelays[i][0] != DefaultValue)
        {
            configFile->print(RelayLink);
            configFile->print(":");
            configFile->print(config->relay.linkedRelays[i][0]);
            configFile->print("=");
            configFile->println(config->relay.linkedRelays[i][1]);
        }
    }

    // R10 - Relay action types
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        if (config->relay.relays[i].actionType != RelayActionType::Default)
        {
            configFile->print(RelaySetActionType);
            configFile->print(":");
            configFile->print(i);
            configFile->print("=");
            configFile->println(static_cast<uint8_t>(config->relay.relays[i].actionType));
        }
    }

    // R11 - Relay pins (skip disabled)
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        if (config->relay.relays[i].pin != PinDisabled)
        {
            configFile->print(RelaySetPin);
            configFile->print(":");
            configFile->print(i);
            configFile->print("=");
            configFile->println(config->relay.relays[i].pin);
        }
    }

    // C7 - Vessel type
    configFile->print(ConfigBoatType);
    configFile->print(":v=");
    configFile->println(static_cast<uint8_t>(config->vessel.vesselType));

    // C9 - Sound delay
    configFile->print(ConfigSoundStartDelay);
    configFile->print(":v=");
    configFile->println(config->sound.startDelayMs);

    // C10 - Bluetooth enabled
    configFile->print(ConfigBluetoothEnable);
    configFile->print(":v=");
    configFile->println(config->network.bluetoothEnabled ? "1" : "0");

    // C11 - WiFi enabled
    configFile->print(ConfigWifiEnable);
    configFile->print(":v=");
    configFile->println(config->network.wifiEnabled ? "1" : "0");

    // C12 - WiFi mode
    configFile->print(ConfigWifiMode);
    configFile->print(":v=");
    configFile->println(static_cast<uint8_t>(config->network.accessMode));

    // C13 - WiFi SSID
    configFile->print(ConfigWifiSSID);
    configFile->print(":");
    configFile->println(config->network.ssid);

    // C14 - WiFi password
    configFile->print(ConfigWifiPassword);
    configFile->print(":");
    configFile->println(config->network.password);

    // C15 - WiFi port
    configFile->print(ConfigWifiPort);
    configFile->print(":v=");
    configFile->println(config->network.port);

    // C17 - WiFi AP IP
    configFile->print(ConfigWifiApIpAddress);
    configFile->print(":");
    configFile->println(config->network.apIpAddress);

    // C20 - Timezone offset
    configFile->print(ConfigTimeZoneOffset);
    configFile->print(":v=");
    configFile->println(config->system.timezoneOffset);

    // C21 - MMSI
    configFile->print(ConfigMmsi);
    configFile->print(":");
    configFile->println(config->vessel.mmsi);

    // C22 - Call sign
    configFile->print(ConfigCallSign);
    configFile->print(":");
    configFile->println(config->vessel.callSign);

    // C23 - Home port
    configFile->print(ConfigHomePort);
    configFile->print(":");
    configFile->println(config->vessel.homePort);

    // C24 - LED colors
    configFile->print(ConfigLedColor);
    configFile->print(":t=0;c=0;r=");
    configFile->print(config->led.dayGoodColor[0]);
    configFile->print(";g=");
    configFile->print(config->led.dayGoodColor[1]);
    configFile->print(";b=");
    configFile->println(config->led.dayGoodColor[2]);

    configFile->print(ConfigLedColor);
    configFile->print(":t=0;c=1;r=");
    configFile->print(config->led.dayBadColor[0]);
    configFile->print(";g=");
    configFile->print(config->led.dayBadColor[1]);
    configFile->print(";b=");
    configFile->println(config->led.dayBadColor[2]);

    configFile->print(ConfigLedColor);
    configFile->print(":t=1;c=0;r=");
    configFile->print(config->led.nightGoodColor[0]);
    configFile->print(";g=");
    configFile->print(config->led.nightGoodColor[1]);
    configFile->print(";b=");
    configFile->println(config->led.nightGoodColor[2]);

    configFile->print(ConfigLedColor);
    configFile->print(":t=1;c=1;r=");
    configFile->print(config->led.nightBadColor[0]);
    configFile->print(";g=");
    configFile->print(config->led.nightBadColor[1]);
    configFile->print(";b=");
    configFile->println(config->led.nightBadColor[2]);

    // C25 - LED brightness
    configFile->print(ConfigLedBrightness);
    configFile->print(":t=0;b=");
    configFile->println(config->led.dayBrightness);

    configFile->print(ConfigLedBrightness);
    configFile->print(":t=1;b=");
    configFile->println(config->led.nightBrightness);

    // C26 - LED auto switch
    configFile->print(ConfigLedAutoSwitch);
    configFile->print(":v=");
    configFile->println(config->led.autoSwitch ? "1" : "0");

    // C27 - LED enable states
    configFile->print(ConfigLedEnable);
    configFile->print(":g=");
    configFile->print(config->led.gpsEnabled ? "1" : "0");
    configFile->print(";w=");
    configFile->print(config->led.warningEnabled ? "1" : "0");
    configFile->print(";s=");
    configFile->println(config->led.systemEnabled ? "1" : "0");

    // C28 - Control panel tones
    configFile->print(ControlPanelTones);
    configFile->print(":t=0;h=");
    configFile->print(config->sound.goodToneHz);
    configFile->print(";d=");
    configFile->print(config->sound.goodDurationMs);
    configFile->print(";p=");
    configFile->print(config->sound.goodPreset);
    configFile->println(";r=0");

    configFile->print(ControlPanelTones);
    configFile->print(":t=1;h=");
    configFile->print(config->sound.badToneHz);
    configFile->print(";d=");
    configFile->print(config->sound.badDurationMs);
    configFile->print(";p=");
    configFile->print(config->sound.badPreset);
    configFile->print(";r=");
    configFile->println(config->sound.badRepeatMs);

    sdDriver.closeFile(MicroSdFileHandle::ConfigLoader);
    sdDriver.releaseExclusiveAccess();

    logInfo("Config exported to SD card");

    return true;
}

void SdCardConfigLoader::onSdCardReady(bool isNewCard)
{
    if (isNewCard)
    {
        logInfo("New SD card detected - checking for config...");
    }
    else
    {
        logInfo("SD card initialized - checking for config...");
    }

    bool configLoaded = loadConfigFromSd();

    if (!configLoaded && !isNewCard)
    {
        // Initial boot and no SD config found - request sync from control panel
        if (_configSyncManager)
        {
            _configSyncManager->requestSync();
        }
    }
}

#endif
