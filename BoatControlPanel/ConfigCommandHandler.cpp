#include "ConfigCommandHandler.h"
#include "SystemFunctions.h"
#include "DateTimeManager.h"

ConfigCommandHandler::ConfigCommandHandler(BroadcastManager* broadcastManager, HomePage* homePage)
	: _broadcastManager(broadcastManager), _homePage(homePage)
{
}

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    // Access the in-memory config
    Config* cfg = ConfigManager::getConfigPtr();
    if (!cfg)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    if (strcmp(command, ConfigRename) == 0)
    {
        if (paramCount >= 1)
        {
            if (SystemFunctions::calculateLength(params[0].value) == 0)
            {
                sendAckErr(sender, command, F("Empty name"), &params[0]);
                return true;
            }

            // enforce max length BOAT_NAME_MAX_LEN (defined in ConfigManager / Config.h)
            strncpy(cfg->name, params[0].value, sizeof(cfg->name) - 1);
            cfg->name[sizeof(cfg->name) - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
    else if (strcmp(command, ConfigRenameRelay) == 0)
    {
        // Expect "C4:<idx>=<shortName>" or "C4:<idx>=<shortName|longName>" where idx 0..7
        if (paramCount >= 1)
        {
            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            //String name = params[0].value;
            if (SystemFunctions::calculateLength(params[0].value) == 0)
            {
                // fallback if they sent single token e.g. "RNAME 2" (no name) -> error
                sendAckErr(sender, command, F("Missing name"), &params[0]);
                return true;
            }

            if (idx >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Index out of range"), &params[0]);
                return true;
            }

            // Parse short and long names (format: "shortName|longName" or just "shortName")
            int pipeIndex = SystemFunctions::indexOf(params[0].value, '|', 0);
            char shortName[6] = "";
            char longName[21] = "";

            if (pipeIndex >= 0)
            {
                char tmpShortName[6] = "";
                char tmpLongName[21] = "";
                SystemFunctions::substr(tmpShortName, sizeof(tmpShortName), params[0].value, 0, pipeIndex);
                SystemFunctions::substr(tmpLongName, sizeof(tmpLongName), params[0].value, pipeIndex + 1);
                strncpy(shortName, tmpShortName, sizeof(shortName) - 1);
                shortName[sizeof(shortName) - 1] = '\0';
                strncpy(longName, tmpLongName, sizeof(longName) - 1);
                longName[sizeof(longName) - 1] = '\0';
            }
            else
            {
                strncpy(shortName, params[0].value, sizeof(shortName) - 1);
                shortName[sizeof(shortName) - 1] = '\0';
			}

            // Copy short name with truncation to relay short name length
            size_t maxShortLen = sizeof(cfg->relayShortNames[idx]) - 1;
            strncpy(cfg->relayShortNames[idx], shortName, maxShortLen);
            cfg->relayShortNames[idx][maxShortLen] = '\0';

            // Copy long name with truncation to relay long name length
            size_t maxLongLen = sizeof(cfg->relayLongNames[idx]) - 1;
            strncpy(cfg->relayLongNames[idx], longName, maxLongLen);
            cfg->relayLongNames[idx][maxLongLen] = '\0';

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
        }
    }
    else if (strcmp(command, ConfigMapHomeButton) == 0)
    {
        // Expect "MAP <button>=<relay>" where button 0..3, relay 0..7 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t relay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (button >= ConfigHomeButtons)
            {
                sendAckErr(sender, command, F("Button out of range"), &params[0]);
                return true;
            }

            if (relay >= (int)ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, command, F("Relay out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            cfg->homePageMapping[button] = relay;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
        }
    }
    else if (strcmp(command, ConfigSetButtonColor) == 0)
    {
        // Expect "MAP <button>=<color>" where button 0..3, image 0..5 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t buttonColor = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (buttonColor < 0xFF)
			    buttonColor += 2; // Adjust to match BTN_COLOR_* constants (2..7), 255 to clear

            if (button >= (int)ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Button out of range"), &params[0]);
                return true;
            }
            
            if ((buttonColor < ImageButtonColorBlue || buttonColor > (int)ImageButtonColorYellow) && buttonColor != DefaultValue)
            {
                sendAckErr(sender, command, F("Button out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            cfg->buttonImage[button] = (uint8_t)buttonColor;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }
    }
    else if (strcmp(command, ConfigSoundRelayId) == 0)
    {
        // Expect "C8:<value>=<relay>" where relay 0..7 (or 255 to unmap)
        if (paramCount == 1 && strlen(params[0].value) > 0)
        {
            uint8_t relay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relay >= (int)ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, command, F("Relay out of range (or 255 to clear)"), &params[0]);
                return true;
            }

			cfg->hornRelayIndex = relay;
			char buffer[20];
			snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), relay);
			_broadcastManager->sendCommand(ConfigSoundRelayId, buffer, true);

            // Auto-update the relay name to include "Sound Signals"
            if (relay != DefaultValue)
            {
                // Only auto-rename if the relay does not already have a custom name
                if (cfg->relayShortNames[relay][0] == '\0') {
                    strncpy(cfg->relayShortNames[relay], "Sound", sizeof(cfg->relayShortNames[relay]) - 1);
                    cfg->relayShortNames[relay][sizeof(cfg->relayShortNames[relay]) - 1] = '\0';
                }
                if (cfg->relayLongNames[relay][0] == '\0') {
                    strncpy(cfg->relayLongNames[relay], "Sound\r\nSignals", sizeof(cfg->relayLongNames[relay]) - 1);
                    cfg->relayLongNames[relay][sizeof(cfg->relayLongNames[relay]) - 1] = '\0';
                }
            }

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
        }
    }
    else if (strcmp(command, ConfigSoundStartDelay) == 0)
    {
        // C9 - Set sound start delay in milliseconds
        if (paramCount >= 1)
        {
            uint16_t delay = static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0));
            cfg->soundStartDelayMs = delay;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
#if defined(ARDUINO_UNO_R4)
    else if (strcmp(command, ConfigBluetoothEnable) == 0)
    {
        // C10 - Enable/disable Bluetooth
        if (paramCount >= 1)
        {
            uint8_t enabled = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
            if (enabled > 1)
            {
                sendAckErr(sender, command, F("Invalid value (0 or 1)"), &params[0]);
                return true;
            }
            cfg->bluetoothEnabled = (enabled == 1);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiEnable) == 0)
    {
        // C11 - Enable/disable WiFi
        if (paramCount >= 1)
        {
            uint8_t enabled = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
            if (enabled > 1)
            {
                sendAckErr(sender, command, F("Invalid value (0 or 1)"), &params[0]);
                return true;
            }
            cfg->wifiEnabled = (enabled == 1);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiMode) == 0)
    {
        // C12 - Set WiFi mode (0=AP, 1=Client)
        if (paramCount >= 1)
        {
            uint8_t mode = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
            if (mode > 1)
            {
                sendAckErr(sender, command, F("Invalid mode (0=AP, 1=Client)"), &params[0]);
                return true;
            }
            cfg->accessMode = mode;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiSSID) == 0)
    {
        // C13 - Set WiFi SSID
        if (paramCount >= 1)
        {
            if (cfg->accessMode != static_cast<uint8_t>(WifiMode::Client))
            {
                sendAckErr(sender, command, F("Only available in Client mode"));
                return true;
            }
            strncpy(cfg->apSSID, params[0].value, sizeof(cfg->apSSID) - 1);
            cfg->apSSID[sizeof(cfg->apSSID) - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiPassword) == 0)
    {
        // C14 - Set WiFi password
        if (paramCount >= 1)
        {
            if (cfg->accessMode != static_cast<uint8_t>(WifiMode::Client))
            {
                sendAckErr(sender, command, F("Only available in Client mode"));
                return true;
            }
            strncpy(cfg->apPassword, params[0].value, sizeof(cfg->apPassword) - 1);
            cfg->apPassword[sizeof(cfg->apPassword) - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiPort) == 0)
    {
        // C15 - Set WiFi port
        if (paramCount >= 1)
        {
            uint16_t port = static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0));
            if (port == 0)
            {
                sendAckErr(sender, command, F("Invalid port"), &params[0]);
                return true;
            }
            cfg->wifiPort = port;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigWifiState) == 0)
    {
        // C16 - Get WiFi connection state (read-only, would need WiFi manager reference)
        // This should likely be handled by SystemCommandHandler instead
        sendAckErr(sender, command, F("Not implemented - use SystemCommandHandler"));
    }
    else if (strcmp(command, ConfigWifiApIpAddress) == 0)
    {
        // C17 - Set AP IP address
        if (paramCount >= 1)
        {
            strncpy(cfg->apIpAddress, params[0].value, sizeof(cfg->apIpAddress) - 1);
            cfg->apIpAddress[sizeof(cfg->apIpAddress) - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
#endif
    else if (strcmp(command, ConfigDefaultRelayState) == 0)
    {
        // C18 - Set initial relay state (format: <relay>=<value>)
        if (paramCount >= 1)
        {
            uint8_t relay = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t value = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relay >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Relay out of range"), &params[0]);
                return true;
            }

            if (value > 1)
            {
                sendAckErr(sender, command, F("Invalid value (0 or 1)"), &params[0]);
                return true;
            }

            cfg->defaulRelayState[relay] = (value == 1);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
        }
    }
    else if (strcmp(command, ConfigLinkRelays) == 0)
    {
        // C19 - Link relays together (format: <relay>=<linkedrelay>)
        if (paramCount >= 1)
        {
            uint8_t relay = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t linkedRelay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relay >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Relay out of range"), &params[0]);
                return true;
            }

            if (linkedRelay >= ConfigRelayCount && linkedRelay != 0xFF)
            {
                sendAckErr(sender, command, F("Linked relay out of range (or 255 to unlink)"), &params[0]);
                return true;
            }

            // First check if relay is already linked
            bool alreadyLinked = false;
            int8_t existingIndex = -1;

            for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
            {
                if (cfg->linkedRelays[i][0] == relay)
                {
                    alreadyLinked = true;
                    existingIndex = i;
                    break;
                }
            }

            if (linkedRelay == 0xFF)
            {
                if (alreadyLinked)
                {
                    cfg->linkedRelays[existingIndex][0] = 0xFF;
                    cfg->linkedRelays[existingIndex][1] = 0xFF;
                }
            }
            else
            {
                if (alreadyLinked)
                {
                    sendAckErr(sender, command, F("Relay already linked"));
                    return true;
                }
                
                // Find empty slot
                bool found = false;
                for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
                {
                    if (cfg->linkedRelays[i][0] == 0xFF)
                    {
                        cfg->linkedRelays[i][0] = relay;
                        cfg->linkedRelays[i][1] = linkedRelay;
                        found = true;
                        break;
                    }
                }
                
                if (!found)
                {
                    sendAckErr(sender, command, F("No available link slots"));
                    return true;
                }
            }

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params"));
        }
    }
    else if (strcmp(command, ConfigTimeZoneOffset) == 0)
    {
        // C20 - Set timezone offset
        if (paramCount >= 1)
        {
            int8_t offset = static_cast<int8_t>(strtol(params[0].value, nullptr, 0));
            if (offset < -12 || offset > 14)
            {
                sendAckErr(sender, command, F("Invalid offset (-12 to +14)"), &params[0]);
                return true;
            }
            cfg->timezoneOffset = offset;
            DateTimeManager::setTimezoneOffset(offset);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigMmsi) == 0)
    {
        // C21 - Set MMSI
        if (paramCount >= 1)
        {
            if (SystemFunctions::calculateLength(params[0].value) != 9)
            {
                sendAckErr(sender, command, F("MMSI must be 9 digits"), &params[0]);
                return true;
            }
            strncpy(cfg->mMSI, params[0].value, ConfigMmsiLength - 1);
            cfg->mMSI[ConfigMmsiLength - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigCallSign) == 0)
    {
        // C22 - Set call sign
        if (paramCount >= 1)
        {
            strncpy(cfg->callSign, params[0].value, ConfigCallSignLength - 1);
            cfg->callSign[ConfigCallSignLength - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigHomePort) == 0)
    {
        // C23 - Set home port
        if (paramCount >= 1)
        {
            strncpy(cfg->homePort, params[0].value, ConfigHomePortLength - 1);
            cfg->homePort[ConfigHomePortLength - 1] = '\0';
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigSaveSettings) == 0)
    {
        // Recompute checksum and persist to EEPROM
        bool ok = ConfigManager::save();
        if (ok)
        {
            // Preserve previous explicit SAVED token
            sendAckOk(sender, command);
        }
        else
        {
            sendAckErr(sender, command, F("EEPROM commit failed"));
            return true;
        }
    }
    else if (strcmp(command, ConfigGetSettings) == 0)
    {
        char buffer[128];
        // return summary of config back to caller in multiple commands
        // C1:<name>
        sender->sendCommand(ConfigRename, cfg->name);

        // C4 entries - send both short and long names in format: <idx>=<shortName|longName>
        for (uint8_t i = 0; i < ConfigRelayCount; ++i)
        {
            snprintf_P(buffer, sizeof(buffer), PSTR("%u=%s|%s"), i, cfg->relayShortNames[i], cfg->relayLongNames[i]);
            sender->sendCommand(ConfigRenameRelay, buffer);
        }

        // C5 entries
        for (uint8_t s = 0; s < ConfigHomeButtons; ++s)
        {
            snprintf_P(buffer, sizeof(buffer), PSTR("%u=%u"), s, cfg->homePageMapping[s]);
            sender->sendCommand(ConfigMapHomeButton, buffer);
        }

		// C6 Send home page button color mappings
		for (uint8_t i = 0; i < ConfigRelayCount; i++)
		{
			snprintf_P(buffer, sizeof(buffer), PSTR("%u=%u"), i, cfg->buttonImage[i]);
			sender->sendCommand(ConfigSetButtonColor, buffer);
		}

		// C7 Boat type
		snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), static_cast<int>(cfg->vesselType));
		sender->sendCommand(ConfigBoatType, buffer);

		// C8 Sound relay ID
		snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->hornRelayIndex);
		sender->sendCommand(ConfigSoundRelayId, buffer);

        // C9 Sound start delay
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->soundStartDelayMs);
        sender->sendCommand(ConfigSoundStartDelay, buffer);

#if defined(ARDUINO_UNO_R4)
        // C10 Bluetooth enabled
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->bluetoothEnabled ? 1 : 0);
        sender->sendCommand(ConfigBluetoothEnable, buffer);

        // C11 WiFi enabled
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->wifiEnabled ? 1 : 0);
        sender->sendCommand(ConfigWifiEnable, buffer);

        // C12 WiFi mode
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->accessMode);
        sender->sendCommand(ConfigWifiMode, buffer);

        // C13 WiFi SSID
        sender->sendCommand(ConfigWifiSSID, cfg->apSSID);

        // C14 WiFi password (consider security implications)
        sender->sendCommand(ConfigWifiPassword, cfg->apPassword);

        // C15 WiFi port
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), cfg->wifiPort);
        sender->sendCommand(ConfigWifiPort, buffer);

        // C17 AP IP address
        sender->sendCommand(ConfigWifiApIpAddress, cfg->apIpAddress);
#endif

        // C18 Default relay states
        for (uint8_t i = 0; i < ConfigRelayCount; ++i)
        {
            snprintf_P(buffer, sizeof(buffer), PSTR("%u=%u"), i, cfg->defaulRelayState[i] ? 1 : 0);
            sender->sendCommand(ConfigDefaultRelayState, buffer);
        }

        // C19 Linked relays
        for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
        {
            if (cfg->linkedRelays[i][0] != 0xFF)
            {
                snprintf_P(buffer, sizeof(buffer), PSTR("%u=%u"), cfg->linkedRelays[i][0], cfg->linkedRelays[i][1]);
                sender->sendCommand(ConfigLinkRelays, buffer);
            }
        }

        // C20 Timezone offset
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), static_cast<int8_t>(cfg->timezoneOffset));
        sender->sendCommand(ConfigTimeZoneOffset, buffer);

        // C21 MMSI
        sender->sendCommand(ConfigMmsi, cfg->mMSI);

        // C22 Call sign
        sender->sendCommand(ConfigCallSign, cfg->callSign);

        // C23 Home port
        sender->sendCommand(ConfigHomePort, cfg->homePort);

        // C24 LED colors (send 4 color configs: day good, day bad, night good, night bad)
        snprintf_P(buffer, sizeof(buffer), PSTR("t=0;c=0;r=%u;g=%u;b=%u"), 
            cfg->ledConfig.dayGoodColor[0], cfg->ledConfig.dayGoodColor[1], cfg->ledConfig.dayGoodColor[2]);
        sender->sendCommand(ConfigLedColor, buffer);

        snprintf_P(buffer, sizeof(buffer), PSTR("t=0;c=1;r=%u;g=%u;b=%u"), 
            cfg->ledConfig.dayBadColor[0], cfg->ledConfig.dayBadColor[1], cfg->ledConfig.dayBadColor[2]);
        sender->sendCommand(ConfigLedColor, buffer);

        snprintf_P(buffer, sizeof(buffer), PSTR("t=1;c=0;r=%u;g=%u;b=%u"), 
            cfg->ledConfig.nightGoodColor[0], cfg->ledConfig.nightGoodColor[1], cfg->ledConfig.nightGoodColor[2]);
        sender->sendCommand(ConfigLedColor, buffer);

        snprintf_P(buffer, sizeof(buffer), PSTR("t=1;c=1;r=%u;g=%u;b=%u"), 
            cfg->ledConfig.nightBadColor[0], cfg->ledConfig.nightBadColor[1], cfg->ledConfig.nightBadColor[2]);
        sender->sendCommand(ConfigLedColor, buffer);
        
        // C25 LED brightness
        snprintf_P(buffer, sizeof(buffer), PSTR("t=0;b=%u"), cfg->ledConfig.dayBrightness);
        sender->sendCommand(ConfigLedBrightness, buffer);

        snprintf_P(buffer, sizeof(buffer), PSTR("t=1;b=%u"), cfg->ledConfig.nightBrightness);
        sender->sendCommand(ConfigLedBrightness, buffer);
        
        // C26 LED auto-switch
        snprintf_P(buffer, sizeof(buffer), PSTR("v=%s"), cfg->ledConfig.autoSwitch ? "true" : "false");
        sender->sendCommand(ConfigLedAutoSwitch, buffer);
        
        // C27 LED enable states
        snprintf_P(buffer, sizeof(buffer), PSTR("g=%s;w=%s;s=%s"),
            cfg->ledConfig.gpsEnabled ? "true" : "false",
            cfg->ledConfig.warningEnabled ? "true" : "false",
            cfg->ledConfig.systemEnabled ? "true" : "false");
        sender->sendCommand(ConfigLedEnable, buffer);

        // C28 Control panel tones (send both good and bad tone configs)
        snprintf_P(buffer, sizeof(buffer), PSTR("t=0;h=%u;d=%u;p=%u;r=0"),
            cfg->soundConfig.good_toneHz,
            cfg->soundConfig.good_durationMs,
            cfg->soundConfig.goodPreset);
        sender->sendCommand(ControlPanelTones, buffer);

        snprintf_P(buffer, sizeof(buffer), PSTR("t=1;h=%u;d=%u;p=%u;r=%lu"),
            cfg->soundConfig.bad_toneHz,
            cfg->soundConfig.bad_durationMs,
            cfg->soundConfig.badPreset,
            cfg->soundConfig.bad_repeatMs);
        sender->sendCommand(ControlPanelTones, buffer);

        sendAckOk(sender, command);
    }
    else if (strcmp(command, ConfigBoatType) == 0)
    {
        // Expect "C7:type=<value>" where value is 0..3
        if (paramCount >= 1)
        {
            uint8_t type = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
            if (type > static_cast<uint8_t>(VesselType::Yacht))
            {
                sendAckErr(sender, command, F("Invalid boat type"), &params[0]);
                return true;
            }
            cfg->vesselType = static_cast<VesselType>(type);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
	}
	else if (strcmp(command, ConfigResetSettings) == 0)
    {
        // Reset to defaults
        ConfigManager::resetToDefaults();
        sendAckOk(sender, command);
	}
    else if (strcmp(command, ConfigLedColor) == 0)
    {
        // Expect "C24:t=0;r=255;g=50;b=213" where t=0 (day) or t=1 (night)
        if (paramCount >= 5)
        {
            uint8_t type = 0;
            uint8_t colorSet = 0;
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;

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

            if (type > 1)
            {
                sendAckErr(sender, command, F("Invalid type (0=day, 1=night)"), &params[0]);
                return true;
            }

            if (colorSet > 1)
            {
                sendAckErr(sender, command, F("Invalid color set (0=good, 1=bad)"), &params[0]);
                return true;
			}
            
            // Check which color set to update based on param key
            bool isGoodColor = colorSet == 0;
            
            if (type == 0) // Day mode
            {
                if (isGoodColor)
                {
                    cfg->ledConfig.dayGoodColor[0] = r;
                    cfg->ledConfig.dayGoodColor[1] = g;
                    cfg->ledConfig.dayGoodColor[2] = b;
                }
                else
                {
                    cfg->ledConfig.dayBadColor[0] = r;
                    cfg->ledConfig.dayBadColor[1] = g;
                    cfg->ledConfig.dayBadColor[2] = b;
                }
            }
            else // Night mode
            {
                if (isGoodColor)
                {
                    cfg->ledConfig.nightGoodColor[0] = r;
                    cfg->ledConfig.nightGoodColor[1] = g;
                    cfg->ledConfig.nightGoodColor[2] = b;
                }
                else
                {
                    cfg->ledConfig.nightBadColor[0] = r;
                    cfg->ledConfig.nightBadColor[1] = g;
                    cfg->ledConfig.nightBadColor[2] = b;
                }
            }
            
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params (t,r,g,b)"));
        }
    }
    else if (strcmp(command, ConfigLedBrightness) == 0)
    {
        // Expect "C25:t=0;b=75" where t=0 (day) or t=1 (night), b=0-100
        if (paramCount >= 2)
        {
            uint8_t type = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
            uint8_t brightness = static_cast<uint8_t>(strtoul(params[1].value, nullptr, 0));
            
            if (type > 1)
            {
                sendAckErr(sender, command, F("Invalid type (0=day, 1=night)"), &params[0]);
                return true;
            }
            
            if (brightness > 100)
            {
                sendAckErr(sender, command, F("Brightness must be 0-100"), &params[1]);
                return true;
            }
            
            if (type == 0)
                cfg->ledConfig.dayBrightness = brightness;
            else
                cfg->ledConfig.nightBrightness = brightness;
            
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params (t,b)"));
        }
    }
    else if (strcmp(command, ConfigLedAutoSwitch) == 0)
    {
        // Expect "C26:v=true" or "C26:v=1"
        if (paramCount >= 1)
        {
            bool autoSwitch = false;
            if (strcmp(params[0].value, "true") == 0 || strcmp(params[0].value, "1") == 0)
                autoSwitch = true;
            else if (strcmp(params[0].value, "false") == 0 || strcmp(params[0].value, "0") == 0)
                autoSwitch = false;
            else
            {
                sendAckErr(sender, command, F("Invalid value (true/false or 0/1)"), &params[0]);
                return true;
            }
            
            cfg->ledConfig.autoSwitch = autoSwitch;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
        }
    }
    else if (strcmp(command, ConfigLedEnable) == 0)
    {
        // Expect "C27:g=true;w=true;s=false"
        if (paramCount >= 3)
        {
            bool gpsEnabled = false;
            bool warningEnabled = false;
            bool systemEnabled = false;

            for (uint8_t i = 0; i < paramCount; ++i)
            {
                bool value = (strcmp(params[i].value, "true") == 0 || strcmp(params[i].value, "1") == 0);

                if (strcmp(params[i].key, "g") == 0)
                    gpsEnabled = value;
                else if (strcmp(params[i].key, "w") == 0)
                    warningEnabled = value;
                else if (strcmp(params[i].key, "s") == 0)
                    systemEnabled = value;
            }

            cfg->ledConfig.gpsEnabled = gpsEnabled;
            cfg->ledConfig.warningEnabled = warningEnabled;
            cfg->ledConfig.systemEnabled = systemEnabled;

            sendAckOk(sender, command);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params (g,w,s)"));
        }
    }
    else if (strcmp(command, ControlPanelTones) == 0)
    {
        // Expect "C28:t=0;h=400;d=500;p=0;r=30000"
        // t = type (0=good, 1=bad)
        // h = tone Hz
        // d = duration in milliseconds
        // p = preset (0=custom, 1=submarine ping, 2=double beep, 3=rising chirp, 4=descending alert, 5=nautical bell, 0xFFFF=no sound)
        // r = repeat interval in milliseconds (0=no repeat, only for bad sounds)
        if (paramCount >= 5)
        {
            uint8_t type = 0;
            uint16_t hz = 0;
            uint16_t duration = 0;
            uint8_t preset = 0;
            uint32_t repeat = 0;

            for (uint8_t i = 0; i < paramCount; ++i)
            {
                if (strcmp(params[i].key, "t") == 0)
                    type = static_cast<uint8_t>(strtoul(params[i].value, nullptr, 0));
                else if (strcmp(params[i].key, "h") == 0)
                    hz = static_cast<uint16_t>(strtoul(params[i].value, nullptr, 0));
                else if (strcmp(params[i].key, "d") == 0)
                    duration = static_cast<uint16_t>(strtoul(params[i].value, nullptr, 0));
                else if (strcmp(params[i].key, "p") == 0)
                    preset = static_cast<uint8_t>(strtoul(params[i].value, nullptr, 0));
                else if (strcmp(params[i].key, "r") == 0)
                    repeat = static_cast<uint32_t>(strtoul(params[i].value, nullptr, 0));
            }

            if (type > 1)
            {
                sendAckErr(sender, command, F("Invalid type (0=good, 1=bad)"), &params[0]);
                return true;
            }

            // Store settings based on type
            if (type == 0) // Good tone
            {
                cfg->soundConfig.goodPreset = preset;
                cfg->soundConfig.good_toneHz = hz;
                cfg->soundConfig.good_durationMs = duration;
            }
            else // Bad tone
            {
                cfg->soundConfig.badPreset = preset;
                cfg->soundConfig.bad_toneHz = hz;
                cfg->soundConfig.bad_durationMs = duration;
                cfg->soundConfig.bad_repeatMs = repeat;
            }

            sendAckOk(sender, command);
        }
        else
        {
            sendAckErr(sender, command, F("Missing params (t,h,d,p,r)"));
        }
    }
    else
    {
        sendAckErr(sender, command, F("Unknown config command"));
    }

    // Notify UI
    if (_homePage)
        _homePage->configUpdated();

    return true;
}

const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
	static const char* cmds[] = { 
		ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings, ConfigRename,
		ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor, ConfigBoatType,
		ConfigSoundRelayId, ConfigSoundStartDelay, ConfigDefaultRelayState, ConfigLinkRelays,
		ConfigTimeZoneOffset, ConfigMmsi, ConfigCallSign, ConfigHomePort, ConfigLedColor,
		ConfigLedBrightness, ConfigLedAutoSwitch, ConfigLedEnable, ControlPanelTones
#if defined(ARDUINO_UNO_R4)
		, ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID,
		ConfigWifiPassword, ConfigWifiPort, ConfigWifiApIpAddress
#endif
	};
	count = sizeof(cmds) / sizeof(cmds[0]);
	return cmds;
}