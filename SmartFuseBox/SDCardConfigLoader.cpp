#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include "SDCardConfigLoader.h"
#include "ConfigManager.h"
#include <string.h>

SdCardConfigLoader::SdCardConfigLoader(SerialCommandManager* computerSerial,
                                       SerialCommandManager* linkSerial,
                                       ConfigController* configController,
                                       ConfigSyncManager* configSyncManager)
    : _computerSerial(computerSerial),
      _linkSerial(linkSerial),
      _configController(configController),
      _configSyncManager(configSyncManager),
      _sdConfigPresent(false)
{
}

bool SdCardConfigLoader::checkSdCard()
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
    return sdDriver.isReady();
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
    while (end > buffer && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t'))
    {
        *end = '\0';
        end--;
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

    // Now apply the command through ConfigController
    // We bypass the SerialCommandManager and call ConfigController methods directly
    
    ConfigResult result = ConfigResult::InvalidCommand;

    if (strcmp(command, "C3") == 0 && paramCount >= 1)
    {
        result = _configController->rename(params[0].value);
    }
    else if (strcmp(command, "C4") == 0 && paramCount >= 1)
    {
        uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        result = _configController->renameRelay(idx, params[0].value);
    }
    else if (strcmp(command, "C5") == 0 && paramCount >= 1)
    {
        uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t relay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
        result = _configController->mapHomeButton(button, relay);
    }
    else if (strcmp(command, "C6") == 0 && paramCount >= 1)
    {
        uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t color = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
        result = _configController->mapHomeButtonColor(button, color);
    }
    else if (strcmp(command, "C7") == 0 && paramCount >= 1)
    {
        uint8_t type = atoi(params[0].value);
        result = _configController->setVesselType(type);
    }
    else if (strcmp(command, "C8") == 0 && paramCount >= 1)
    {
        uint8_t relay = atoi(params[0].value);
        result = _configController->setSoundRelayButton(relay);
    }
    else if (strcmp(command, "C9") == 0 && paramCount >= 1)
    {
        uint16_t delay = atoi(params[0].value);
        result = _configController->setsoundDelayStart(delay);
    }
    else if (strcmp(command, "C10") == 0 && paramCount >= 1)
    {
        bool enabled = (atoi(params[0].value) != 0);
        result = _configController->setBluetoothEnabled(enabled);
    }
    else if (strcmp(command, "C11") == 0 && paramCount >= 1)
    {
        bool enabled = (atoi(params[0].value) != 0);
        result = _configController->setWifiEnabled(enabled);
    }
    else if (strcmp(command, "C12") == 0 && paramCount >= 1)
    {
        uint8_t mode = atoi(params[0].value);
        result = _configController->setWifiAccessMode(mode);
    }
    else if (strcmp(command, "C13") == 0 && paramCount >= 1)
    {
        // For C13, the entire string after : is the SSID (no v= prefix in file)
        // We need to find original param value from line
        const char* ssid = strchr(line, ':');
        if (ssid)
        {
            ssid++; // Skip the colon
            result = _configController->setWifiSsid(ssid);
        }
    }
    else if (strcmp(command, "C14") == 0 && paramCount >= 1)
    {
        // For C14, the entire string after : is the password
        const char* password = strchr(line, ':');
        if (password)
        {
            password++; // Skip the colon
            result = _configController->setWifiPassword(password);
        }
    }
    else if (strcmp(command, "C15") == 0 && paramCount >= 1)
    {
        uint16_t port = atoi(params[0].value);
        result = _configController->setWifiPort(port);
    }
    else if (strcmp(command, "C17") == 0 && paramCount >= 1)
    {
        // For C17, the entire string after : is the IP address
        const char* ip = strchr(line, ':');
        if (ip)
        {
            ip++; // Skip the colon
            result = _configController->setWifiIpAddress(ip);
        }
    }
    else if (strcmp(command, "C18") == 0 && paramCount >= 1)
    {
        uint8_t relay = static_cast<uint8_t>(atoi(params[0].key));
        bool state = (atoi(params[0].value) != 0);
        result = _configController->setRelayDefaultState(relay, state);
    }
    else if (strcmp(command, "C19") == 0 && paramCount >= 1)
    {
        uint8_t relay1 = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
        uint8_t relay2 = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
        
        if (relay2 == 255)
        {
            result = _configController->unlinkRelay(relay1);
        }
        else
        {
            result = _configController->linkRelays(relay1, relay2);
        }
    }
    else if (strcmp(command, "C20") == 0 && paramCount >= 1)
    {
        int8_t offset = static_cast<int8_t>(atoi(params[0].value));
        result = _configController->setTimezoneOffset(offset);
    }
    else if (strcmp(command, "C21") == 0 && paramCount >= 1)
    {
        const char* mmsi = strchr(line, ':');
        if (mmsi)
        {
            mmsi++; // Skip the colon
            result = _configController->setMmsi(mmsi);
        }
    }
    else if (strcmp(command, "C22") == 0)
    {
        const char* callSign = strchr(line, ':');
        if (callSign && *(callSign + 1) != '\0')
        {
            callSign++; // Skip the colon
            result = _configController->setCallSign(callSign);
        }
        else
        {
            result = _configController->setCallSign("");
        }
    }
    else if (strcmp(command, "C23") == 0)
    {
        const char* homePort = strchr(line, ':');
        if (homePort && *(homePort + 1) != '\0')
        {
            homePort++; // Skip the colon
            result = _configController->setHomePort(homePort);
        }
        else
        {
            result = _configController->setHomePort("");
        }
    }
    else if (strcmp(command, "C24") == 0 && paramCount >= 5)
    {
        uint8_t type = 0, colorSet = 0, r = 0, g = 0, b = 0;
        
        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = atoi(params[i].value);
            else if (strcmp(params[i].key, "c") == 0)
                colorSet = atoi(params[i].value);
            else if (strcmp(params[i].key, "r") == 0)
                r = atoi(params[i].value);
            else if (strcmp(params[i].key, "g") == 0)
                g = atoi(params[i].value);
            else if (strcmp(params[i].key, "b") == 0)
                b = atoi(params[i].value);
        }
        
        result = _configController->setLedColor(type, colorSet, r, g, b);
    }
    else if (strcmp(command, "C25") == 0 && paramCount >= 2)
    {
        uint8_t type = 0, brightness = 0;
        
        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = atoi(params[i].value);
            else if (strcmp(params[i].key, "b") == 0)
                brightness = atoi(params[i].value);
        }
        
        result = _configController->setLedBrightness(type, brightness);
    }
    else if (strcmp(command, "C26") == 0 && paramCount >= 1)
    {
        bool enabled = (atoi(params[0].value) != 0);
        result = _configController->setLedAutoSwitch(enabled);
    }
    else if (strcmp(command, "C27") == 0 && paramCount >= 3)
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
    else if (strcmp(command, "C28") == 0 && paramCount >= 4)
    {
        uint8_t type = 0, preset = 0;
        uint16_t toneHz = 0, durationMs = 0;
        uint32_t repeatMs = 0;
        
        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "t") == 0)
                type = atoi(params[i].value);
            else if (strcmp(params[i].key, "h") == 0)
                toneHz = atoi(params[i].value);
            else if (strcmp(params[i].key, "d") == 0)
                durationMs = atoi(params[i].value);
            else if (strcmp(params[i].key, "p") == 0)
                preset = atoi(params[i].value);
            else if (strcmp(params[i].key, "r") == 0)
                repeatMs = strtoul(params[i].value, nullptr, 0);
        }
        
        result = _configController->setControlPanelTones(type, preset, toneHz, durationMs, repeatMs);
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
        logInfo("SD card not present or not accessible");

        return false;
    }

    if (!configFileExists())
    {
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
                                           SD_CONFIG_FILENAME, O_WRONLY | O_CREAT);
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
    if (strlen(config->name) > 0)
    {
        configFile->print("C3:");
        configFile->println(config->name);
    }

    // C4 - Relay names
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print("C4:");
        configFile->print(i);
        configFile->print("=");
        configFile->print(config->relayShortNames[i]);
        configFile->print("|");
        configFile->println(config->relayLongNames[i]);
    }

    // C5 - Home button mappings
    for (uint8_t i = 0; i < ConfigHomeButtons; i++)
    {
        configFile->print("C5:");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->homePageMapping[i]);
    }

    // C6 - Button colors
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print("C6:");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->buttonImage[i]);
    }

    // C7 - Vessel type
    configFile->print("C7:v=");
    configFile->println(static_cast<uint8_t>(config->vesselType));

    // C8 - Sound relay
    configFile->print("C8:v=");
    configFile->println(config->hornRelayIndex);

    // C9 - Sound delay
    configFile->print("C9:v=");
    configFile->println(config->soundStartDelayMs);

    // C10 - Bluetooth enabled
    configFile->print("C10:v=");
    configFile->println(config->bluetoothEnabled ? "1" : "0");

    // C11 - WiFi enabled
    configFile->print("C11:v=");
    configFile->println(config->wifiEnabled ? "1" : "0");

    // C12 - WiFi mode
    configFile->print("C12:v=");
    configFile->println(config->accessMode);

    // C13 - WiFi SSID
    configFile->print("C13:");
    configFile->println(config->apSSID);

    // C14 - WiFi password
    configFile->print("C14:");
    configFile->println(config->apPassword);

    // C15 - WiFi port
    configFile->print("C15:v=");
    configFile->println(config->wifiPort);

    // C17 - WiFi AP IP
    configFile->print("C17:");
    configFile->println(config->apIpAddress);

    // C18 - Default relay states
    for (uint8_t i = 0; i < ConfigRelayCount; i++)
    {
        configFile->print("C18:");
        configFile->print(i);
        configFile->print("=");
        configFile->println(config->defaulRelayState[i] ? "1" : "0");
    }

    // C19 - Linked relays
    for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
    {
        configFile->print("C19:");
        configFile->print(config->linkedRelays[i][0]);
        configFile->print("=");
        configFile->println(config->linkedRelays[i][1]);
    }

    // C20 - Timezone offset
    configFile->print("C20:v=");
    configFile->println(config->timezoneOffset);

    // C21 - MMSI
    configFile->print("C21:");
    configFile->println(config->mMSI);

    // C22 - Call sign
    configFile->print("C22:");
    configFile->println(config->callSign);

    // C23 - Home port
    configFile->print("C23:");
    configFile->println(config->homePort);

    // C24 - LED colors
    configFile->print("C24:t=0;c=0;r=");
    configFile->print(config->ledConfig.dayGoodColor[0]);
    configFile->print(";g=");
    configFile->print(config->ledConfig.dayGoodColor[1]);
    configFile->print(";b=");
    configFile->println(config->ledConfig.dayGoodColor[2]);

    configFile->print("C24:t=0;c=1;r=");
    configFile->print(config->ledConfig.dayBadColor[0]);
    configFile->print(";g=");
    configFile->print(config->ledConfig.dayBadColor[1]);
    configFile->print(";b=");
    configFile->println(config->ledConfig.dayBadColor[2]);

    configFile->print("C24:t=1;c=0;r=");
    configFile->print(config->ledConfig.nightGoodColor[0]);
    configFile->print(";g=");
    configFile->print(config->ledConfig.nightGoodColor[1]);
    configFile->print(";b=");
    configFile->println(config->ledConfig.nightGoodColor[2]);

    configFile->print("C24:t=1;c=1;r=");
    configFile->print(config->ledConfig.nightBadColor[0]);
    configFile->print(";g=");
    configFile->print(config->ledConfig.nightBadColor[1]);
    configFile->print(";b=");
    configFile->println(config->ledConfig.nightBadColor[2]);

    // C25 - LED brightness
    configFile->print("C25:t=0;b=");
    configFile->println(config->ledConfig.dayBrightness);

    configFile->print("C25:t=1;b=");
    configFile->println(config->ledConfig.nightBrightness);

    // C26 - LED auto switch
    configFile->print("C26:v=");
    configFile->println(config->ledConfig.autoSwitch ? "1" : "0");

    // C27 - LED enable states
    configFile->print("C27:g=");
    configFile->print(config->ledConfig.gpsEnabled ? "1" : "0");
    configFile->print(";w=");
    configFile->print(config->ledConfig.warningEnabled ? "1" : "0");
    configFile->print(";s=");
    configFile->println(config->ledConfig.systemEnabled ? "1" : "0");

    // C28 - Control panel tones
    configFile->print("C28:t=0;h=");
    configFile->print(config->soundConfig.good_toneHz);
    configFile->print(";d=");
    configFile->print(config->soundConfig.good_durationMs);
    configFile->print(";p=");
    configFile->print(config->soundConfig.goodPreset);
    configFile->println(";r=0");

    configFile->print("C28:t=1;h=");
    configFile->print(config->soundConfig.bad_toneHz);
    configFile->print(";d=");
    configFile->print(config->soundConfig.bad_durationMs);
    configFile->print(";p=");
    configFile->print(config->soundConfig.badPreset);
    configFile->print(";r=");
    configFile->println(config->soundConfig.bad_repeatMs);

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
