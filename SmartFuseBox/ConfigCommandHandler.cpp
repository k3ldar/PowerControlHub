#include "ConfigCommandHandler.h"
#include "BluetoothController.h"


ConfigCommandHandler::ConfigCommandHandler(SoundManager* soundManager, BluetoothController* bluetoothController)
    : _soundManager(soundManager),
      _bluetoothController(bluetoothController)
{
}

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    // Access the in-memory config
    Config* config = ConfigManager::getConfigPtr();

    if (!config)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    // Normalize command
    String cmd = command;
    cmd.trim();

    if (cmd == ConfigSaveSettings)
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
        // C7 Boat type
        sender->sendCommand(ConfigBoatType, "v=" + String(static_cast<uint8_t>(config->vesselType)));

        // C8 Sound relay ID
        sender->sendCommand(ConfigSoundRelayId, "v=" + String(config->hornRelayIndex));

        // C9 Sound start delay
        sender->sendCommand(ConfigSoundStartDelay, "v=" + String(config->soundStartDelayMs));

		// C10 Bluetooth enable
		sender->sendCommand(ConfigBluetoothEnable, "v=" + String(config->bluetoothEnabled ? "1" : "0"));

        sendAckOk(sender, cmd);
    }
    else if (cmd == ConfigResetSettings)
    {
        // Reset to defaults
        ConfigManager::resetToDefaults();
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

            config->vesselType = static_cast<VesselType>(type);

            updateSoundManagerConfig(config);
            
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
    }
    else if (cmd == ConfigSoundRelayId)
    {
        // Expect "MAP <value>=<relay>" where relay 0..7 (or 255 to unmap)
        if (paramCount == 1 && params[0].value.length() > 0)
        {
            uint8_t relay = params[0].value.toInt();

            if (relay >= ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, cmd, F("Relay out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            config->hornRelayIndex = relay;
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid params"));
        }
    }
    else if (cmd == ConfigSoundStartDelay)
    {
        if (paramCount == 1)
        {
            uint16_t soundStartDelay = params[0].value.toInt();
            config->soundStartDelayMs = soundStartDelay;

            updateSoundManagerConfig(config);

            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid parameters"));
        }
    }
	else if (cmd == ConfigBluetoothEnable)
    {
        // Expect "C10:enable=<0|1>"
        if (paramCount >= 1)
        {
            bool enable = (params[0].value == "1");
            config->bluetoothEnabled = enable;

			// do not apply live, only on next restart, otherwise too many enabled/disable cycles will 
            // eventually force the board to run out of memory, the only exception to this is if 
			// it starts disabled and is being enabled now, i.e. once on it needs rebooting to turn off again
            if (_bluetoothController && !_bluetoothController->isEnabled())
            {
                if (!_bluetoothController->setEnabled(enable))
                {
                    sendAckErr(sender, cmd, F("Bluetooth init failed"), &params[0]);
                    return true;
                }
            }

            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown config command"));
    }

    return true;
}

const String* ConfigCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { ConfigSaveSettings, ConfigGetSettings, 
        ConfigResetSettings, ConfigBoatType, ConfigSoundRelayId, ConfigSoundStartDelay,
        ConfigBluetoothEnable};
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

void ConfigCommandHandler::updateSoundManagerConfig(Config* config)
{
    if (_soundManager != nullptr && config != nullptr)
    {
        _soundManager->configUpdated(config);
    }
}