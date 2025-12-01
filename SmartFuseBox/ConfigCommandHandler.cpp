#include "ConfigCommandHandler.h"
#include "BluetoothController.h"


ConfigCommandHandler::ConfigCommandHandler(SoundController* soundController, BluetoothController* bluetoothController,
    WifiController* wifiController, RelayCommandHandler* relayCommandHandler)
    : _soundController(soundController),
      _bluetoothController(bluetoothController),
	  _wifiController(wifiController),
      _relayCommandHandler(relayCommandHandler)
{
}

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
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

#if defined(ARDUINO_UNO_R4)
		// C10 Bluetooth enable
		sender->sendCommand(ConfigBluetoothEnable, "v=" + String(config->bluetoothEnabled ? "1" : "0"));

		// C11 WiFi enable
		sender->sendCommand(ConfigWifiEnable, "v=" + String(config->wifiEnabled ? "1" : "0"));

		// C12 WiFi mode
		sender->sendCommand(ConfigWifiMode, "v=" + String(config->accessMode));

		// C13 WiFi SSID
		sender->sendCommand(ConfigWifiSSID, "v=" + String(config->_apSSID));

		// C14 WiFi Password
		sender->sendCommand(ConfigWifiPassword, "v=" + String(config->_apPassword));

		// C15 WiFi Port
		sender->sendCommand(ConfigWifiPort, "v=" + String(config->wifiPort));
#endif

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

            updateSoundControllerConfig(config);
            
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

            if (_relayCommandHandler)
            {
				_relayCommandHandler->configUpdated(config);
            }

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

            updateSoundControllerConfig(config);

            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid parameters"));
        }
    }
	else if (cmd == ConfigBluetoothEnable)
    {
        // Expect "C10:v=<0|1>"
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
#if defined(ARDUINO_UNO_R4)
    else if (cmd == ConfigWifiEnable)
    {
        // Expect "C11:v=<0|1>"
        if (paramCount >= 1)
        {
            bool enable = (params[0].value == "1");
            config->wifiEnabled = enable;
            // do not apply live, only on next restart, otherwise too many enabled/disable cycles will 
            // eventually force the board to run out of memory, the only exception to this is if 
            // it starts disabled and is being enabled now, i.e. once on it needs rebooting to turn off again
            if (_wifiController && !_wifiController->isEnabled())
            {
                if (!_wifiController->setEnabled(enable))
                {
                    sendAckErr(sender, cmd, F("WiFi init failed"), &params[0]);
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
    else if (cmd == ConfigWifiMode)
    {
        // Expect "C12:v=<0|1>"
        if (paramCount >= 1)
        {
            uint8_t mode = params[0].value.toInt();
            if (mode > 1)
            {
                sendAckErr(sender, cmd, F("Invalid WiFi mode"), &params[0]);
                return true;
            }
            config->accessMode = mode;
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
	}
	else if (cmd == ConfigWifiSSID)
    {
        // Expect "C13:v=<value>"
        if (paramCount >= 1)
        {
            String ssid = params[0].value;
            ssid.trim();
            ssid.toCharArray(config->_apSSID, sizeof(config->_apSSID));
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
    }
    else if (cmd == ConfigWifiPassword)
    {
        // Expect "C14:v=<value>"
        if (paramCount >= 1)
        {
            String password = params[0].value;
            password.trim();
            password.toCharArray(config->_apPassword, sizeof(config->_apPassword));
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
    }
    else if (cmd == ConfigWifiPort)
    {
        // Expect "C15:v=<value>"
        if (paramCount >= 1)
        {
            uint16_t port = params[0].value.toInt();
            if (port == 0)
            {
                sendAckErr(sender, cmd, F("Invalid port"), &params[0]);
                return true;
            }
            config->wifiPort = port;
            sendAckOk(sender, cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Missing param"));
            return true;
        }
	}
#endif

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
        ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID, 
        ConfigWifiPassword, ConfigWifiPort };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

void ConfigCommandHandler::updateSoundControllerConfig(Config* config)
{
    if (_soundController != nullptr && config != nullptr)
    {
        _soundController->configUpdated(config);
    }
}