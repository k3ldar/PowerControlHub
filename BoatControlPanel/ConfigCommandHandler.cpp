#include "ConfigCommandHandler.h"
#include "SystemFunctions.h"

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

    if (strcmp(command, ConfigRenameBoat) == 0)
    {
        if (paramCount >= 1)
        {
            // Expect "C3:name=<value>" where value is the boat name (or just a single token)
            String name = params[0].value;
            if (name.length() == 0)
                name = params[0].key;

            name.trim();
            if (name.length() == 0)
            {
                sendAckErr(sender, command, F("Empty name"), &params[0]);
                return true;
            }

            // enforce max length BOAT_NAME_MAX_LEN (defined in ConfigManager / Config.h)
            strncpy(cfg->boatName, name.c_str(), sizeof(cfg->boatName) - 1);
            cfg->boatName[sizeof(cfg->boatName) - 1] = '\0';
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
            String name = params[0].value;
            if (name.length() == 0)
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
            int pipeIndex = name.indexOf('|');
            String shortName;
            String longName;

            if (pipeIndex >= 0)
            {
                // Pipe character found - split into short and long names
                shortName = name.substring(0, pipeIndex);
                longName = name.substring(pipeIndex + 1);
                shortName.trim();
                longName.trim();
            }
            else
            {
                // No pipe character - use the same name for both short and long
                shortName = name;
                longName = name;
                shortName.trim();
                longName.trim();
            }

            // Copy short name with truncation to relay short name length
            size_t maxShortLen = sizeof(cfg->relayShortNames[idx]) - 1;
            strncpy(cfg->relayShortNames[idx], shortName.c_str(), maxShortLen);
            cfg->relayShortNames[idx][maxShortLen] = '\0';

            // Copy long name with truncation to relay long name length
            size_t maxLongLen = sizeof(cfg->relayLongNames[idx]) - 1;
            strncpy(cfg->relayLongNames[idx], longName.c_str(), maxLongLen);
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
			_broadcastManager->sendCommand(ConfigSoundRelayId, "v=" + String(relay), true);

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
        sender->sendCommand(ConfigRenameBoat, cfg->boatName);

        // C4 entries - send both short and long names in format: <idx>=<shortName|longName>
        for (uint8_t i = 0; i < ConfigRelayCount; ++i)
        {
            snprintf(buffer, sizeof(buffer), "%u=%s|%s", i, cfg->relayShortNames[i], cfg->relayLongNames[i]);
            sender->sendCommand(ConfigRenameRelay, buffer);
        }

        // C5 entries
        for (uint8_t s = 0; s < ConfigHomeButtons; ++s)
        {
            snprintf(buffer, sizeof(buffer), "%u=%u", s, cfg->homePageMapping[s]);
            sender->sendCommand(ConfigMapHomeButton, buffer);
        }

        // C6 Send home page button color mappings
        for (uint8_t i = 0; i < ConfigRelayCount; i++)
        {
			snprintf(buffer, sizeof(buffer), "%u=%u", i, cfg->buttonImage[i]);
            sender->sendCommand(ConfigSetButtonColor, buffer);
        }

		// C7 Boat type
        snprintf(buffer, sizeof(buffer), "v=%u", static_cast<int>(cfg->vesselType));
		sender->sendCommand(ConfigBoatType, buffer);

		// C8 Sound relay ID
        snprintf(buffer, sizeof(buffer), "v=%u", cfg->hornRelayIndex);
        sender->sendCommand(ConfigSoundRelayId, buffer);

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
    static const char* cmds[] = { ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings, ConfigRenameBoat,
        ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor, ConfigBoatType, ConfigSoundRelayId };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}