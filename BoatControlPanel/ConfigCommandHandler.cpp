#include "ConfigCommandHandler.h"


ConfigCommandHandler::ConfigCommandHandler(BroadcastManager* broadcastManager, HomePage* homePage)
	: _broadcastManager(broadcastManager), _homePage(homePage)
{
}

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    // Access the in-memory config
    Config* cfg = ConfigManager::getConfigPtr();
    if (!cfg)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    // Normalize command
    String cmd = command;
    cmd.trim();

    if (cmd == ConfigRenameBoat)
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
                sendAckErr(sender, cmd, F("Empty name"), &params[0]);
                return true;
            }

            // enforce max length BOAT_NAME_MAX_LEN (defined in ConfigManager / Config.h)
            strncpy(cfg->boatName, name.c_str(), sizeof(cfg->boatName) - 1);
            cfg->boatName[sizeof(cfg->boatName) - 1] = '\0';
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
    }
    else if (cmd == ConfigRenameRelay)
    {
        // Expect "C4:<idx>=<shortName>" or "C4:<idx>=<shortName|longName>" where idx 0..7
        if (paramCount >= 1)
        {
            uint8_t idx = params[0].key.toInt();
            String name = params[0].value;
            if (name.length() == 0)
            {
                // fallback if they sent single token e.g. "RNAME 2" (no name) -> error
                sendAckErr(sender, cmd, F("Missing name"), &params[0]);
                return true;
            }

            if (idx >= ConfigRelayCount)
            {
                sendAckErr(sender, cmd, F("Index out of range"), &params[0]);
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

            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing params"));
        }
    }
    else if (cmd == ConfigMapHomeButton)
    {
        // Expect "MAP <button>=<relay>" where button 0..3, relay 0..7 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = params[0].key.toInt();
            uint8_t relay = params[0].value.toInt(); // if value empty, toInt() -> 0

            if (button >= ConfigHomeButtons)
            {
                sendAckErr(sender, cmd, F("Button out of range"), &params[0]);
                return true;
            }

            if (relay >= (int)ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, cmd, F("Relay out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            cfg->homePageMapping[button] = relay;
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing params"));
        }
    }
    else if (command == ConfigSetButtonColor) 
    {
        // Expect "MAP <button>=<color>" where button 0..3, image 0..5 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = params[0].key.toInt();
            uint8_t buttonColor = params[0].value.toInt();

            if (buttonColor < 0xFF)
			    buttonColor += 2; // Adjust to match BTN_COLOR_* constants (2..7), 255 to clear

            if (button >= (int)ConfigRelayCount)
            {
                sendAckErr(sender, cmd, F("Button out of range"), &params[0]);
                return true;
            }
            
            if ((buttonColor < ImageButtonColorBlue || buttonColor > (int)ImageButtonColorYellow) && buttonColor != DefaultValue)
            {
                sendAckErr(sender, cmd, F("Button out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            cfg->buttonImage[button] = (uint8_t)buttonColor;
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing params"));
            return true;
        }
    }
    else if (cmd == ConfigSoundRelayId)
    {
        // Expect "C8:<value>=<relay>" where relay 0..7 (or 255 to unmap)
        if (paramCount == 1 && params[0].value.length() > 0)
        {
            uint8_t relay = params[0].value.toInt(); // if value empty, toInt() -> 0

            if (relay >= (int)ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, cmd, F("Relay out of range (or 255 to clear)"), &params[0]);
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

            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing params"));
        }
    }
    else if (cmd == ConfigSaveSettings)
    {
        // Recompute checksum and persist to EEPROM
        bool ok = ConfigManager::save();
        if (ok)
        {
            // Preserve previous explicit SAVED token
            sendAckOk(sender, cmd);
        }
        else
        {
            sendAckErr(sender, cmd, F("EEPROM commit failed"));
            return true;
        }
    }
    else if (cmd == ConfigGetSettings)
    {
        // return summary of config back to caller in multiple commands
        // C1:<name>
        sender->sendCommand(ConfigRenameBoat, String(cfg->boatName));

        // C4 entries - send both short and long names in format: <idx>=<shortName|longName>
        for (uint8_t i = 0; i < ConfigRelayCount; ++i)
        {
            String relayNames = String(i) + Equals + String(cfg->relayShortNames[i]) + Pipe + String(cfg->relayLongNames[i]);
            sender->sendCommand(ConfigRenameRelay, relayNames);
        }

        // C5 entries
        for (uint8_t s = 0; s < ConfigHomeButtons; ++s)
        {
            sender->sendCommand(ConfigMapHomeButton, String(s) + Equals + String(cfg->homePageMapping[s]));
        }

        // C6 Send home page button color mappings
        for (uint8_t i = 0; i < ConfigRelayCount; i++)
        {
            String colorMapping = String(i) + Equals + String(cfg->buttonImage[i]);
            sender->sendCommand(ConfigSetButtonColor, colorMapping);
        }

		// C7 Boat type
		sender->sendCommand(ConfigBoatType, String(static_cast<uint8_t>(cfg->vesselType)));

		// C8 Sound relay ID
		sender->sendCommand(ConfigSoundRelayId, String(cfg->hornRelayIndex));

        sendAckOk(sender, cmd);
    }
    else if (cmd == ConfigBoatType)
    {
        // Expect "C7:type=<value>" where value is 0..3
        if (paramCount >= 1)
        {
            uint8_t type = params[0].value.toInt();
            if (type > static_cast<uint8_t>(VesselType::Yacht))
            {
                sendAckErr(sender, cmd, F("Invalid boat type"), &params[0]);
                return true;
            }
            cfg->vesselType = static_cast<VesselType>(type);
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
	}
    else if (cmd == ConfigResetSettings)
    {
        // Reset to defaults
        ConfigManager::resetToDefaults();
        sendAckOk(sender, cmd);
	}
    else
    {
        sendAckErr(sender, cmd, F("Unknown config command"));
    }

    // Notify UI
    if (_homePage)
        _homePage->configUpdated();

    return true;
}

const String* ConfigCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings, ConfigRenameBoat,
        ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor, ConfigBoatType, ConfigSoundRelayId };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}