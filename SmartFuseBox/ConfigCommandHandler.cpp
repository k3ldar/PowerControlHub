#include "ConfigCommandHandler.h"
#include "BluetoothController.h"

#if defined(ARDUINO_UNO_R4)
#include <Arduino.h>
#endif


ConfigCommandHandler::ConfigCommandHandler(SoundController* soundController, BluetoothController* bluetoothController,
    WifiController* wifiController, RelayCommandHandler* relayCommandHandler)
    : _soundController(soundController),
      _bluetoothController(bluetoothController),
	  _wifiController(wifiController),
      _relayCommandHandler(relayCommandHandler)
{
}

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    // Access the in-memory config
    Config* config = ConfigManager::getConfigPtr();

    if (!config)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    if (strcmp(command, ConfigSaveSettings) == 0)
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
		sender->sendCommand(ConfigWifiSSID, "v=" + String(config->apSSID));

		// C14 WiFi Password
		sender->sendCommand(ConfigWifiPassword, "v=" + String(config->apPassword));

		// C15 WiFi Port
		sender->sendCommand(ConfigWifiPort, "v=" + String(config->wifiPort));

		// C16 WiFi State
        if (_wifiController && _wifiController->getServer())
        {
            sender->sendCommand(ConfigWifiState, "v=" + String(static_cast<int32_t>(_wifiController->getServer()->getConnectionState())));
        }

		// C17 WiFi AP IP Address
        if (_wifiController && _wifiController->getServer())
        {
            sender->sendCommand(ConfigWifiApIpAddress, "v=" + _wifiController->getServer()->getIpAddress());
		}
#endif

        sendAckOk(sender, command);
    }
    else if (strcmp(command, ConfigResetSettings) == 0)
    {
        // Reset to defaults
        ConfigManager::resetToDefaults();
        sendAckOk(sender, command);
    }
    else if (strcmp(command, ConfigBoatType) == 0)
    {
        // Expect "C7:type=<value>" where value is 0..3
        if (paramCount >= 1)
        {
            uint8_t type = params[0].value.toInt();
            if (type > static_cast<uint8_t>(VesselType::Yacht))
            {
                sendAckErr(sender, command, F("Invalid boat type"), &params[0]);
                return true;
            }

            config->vesselType = static_cast<VesselType>(type);

            updateSoundControllerConfig(config);
            
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
    else if (strcmp(command, ConfigSoundRelayId) == 0)
    {
        // Expect "MAP <value>=<relay>" where relay 0..7 (or 255 to unmap)
        if (paramCount == 1 && SystemFunctions:: params[0].value.length() > 0)
        {
            uint8_t relay = atoi(params[0].value);

            if (relay >= ConfigRelayCount && relay != DefaultValue)
            {
                sendAckErr(sender, command, F("Relay out of range (or 255 to clear)"), &params[0]);
                return true;
            }

            config->hornRelayIndex = relay;

            if (_relayCommandHandler)
            {
				_relayCommandHandler->configUpdated(config);
            }

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid params"));
        }
    }
    else if (strcmp(command, ConfigSoundStartDelay) == 0)
    {
        if (paramCount == 1)
        {
            uint16_t soundStartDelay = atoi(params[0].value);
            config->soundStartDelayMs = soundStartDelay;

            updateSoundControllerConfig(config);

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
        }
    }
	else if (strcmp(command, ConfigBluetoothEnable) == 0)
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
                    sendAckErr(sender, command, F("Bluetooth init failed"), &params[0]);
                    return true;
                }
            }

            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
#if defined(ARDUINO_UNO_R4)
    else if (strcmp(command, ConfigWifiEnable) == 0)
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
                    sendAckErr(sender, command, F("WiFi init failed"), &params[0]);
                    return true;
                }
            }
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
    else if (strcmp(command, ConfigWifiMode) == 0)
    {
        // Expect "C12:v=<0|1>"
        if (paramCount >= 1)
        {
            uint8_t mode = atoi(params[0].value);
            if (mode > 1)
            {
                sendAckErr(sender, command, F("Invalid WiFi mode"), &params[0]);
                return true;
            }
            config->accessMode = mode;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
	}
	else if (strcmp(command, ConfigWifiSSID) == 0)
    {
        // Expect "C13:v=<value>"
        if (paramCount >= 1)
        {
            String ssid = params[0].value;
            ssid.trim();
            ssid.toCharArray(config->apSSID, sizeof(config->apSSID));
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
    else if (strcmp(command, ConfigWifiPassword) == 0)
    {
        // Expect "C14:v=<value>"
        if (paramCount >= 1)
        {
            String password = params[0].value;
            password.trim();
            password.toCharArray(config->apPassword, sizeof(config->apPassword));
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
    }
    else if (strcmp(command, ConfigWifiPort) == 0)
    {
        // Expect "C15:v=<value>"
        if (paramCount >= 1)
        {
            uint16_t port = atoi(params[0].value);
            if (port == 0)
            {
                sendAckErr(sender, command, F("Invalid port"), &params[0]);
                return true;
            }
            config->wifiPort = port;
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
	}
    else if (strcmp(command, ConfigWifiState) == 0)
    {
        // C16 WiFi State
        uint8_t state = static_cast<uint8_t>(WifiConnectionState::Disconnected);
        if (_wifiController)
        {
            state = static_cast<uint8_t>(_wifiController->getServer()->getConnectionState());
        }
        sender->sendCommand(ConfigWifiState, "v=" + String(state));
        sendAckOk(sender, command);
	}
    else if (strcmp(command, ConfigWifiApIpAddress) == 0)
    {
        // Expect "C17:v=<value>"
        if (paramCount >= 1)
        {
            String ipAddress = params[0].value;
            ipAddress.trim();

            IPAddress ip;
            if (!ip.fromString(ipAddress))
            {
                sendAckErr(sender, command, F("Invalid IP address"), &params[0]);
                return true;
			}

            ipAddress.toCharArray(config->apIpAddress, sizeof(config->apIpAddress));
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Missing param"));
            return true;
        }
	}
#endif

    else
    {
        sendAckErr(sender, command, F("Unknown config command"));
    }

    return true;
}

const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { ConfigSaveSettings, ConfigGetSettings, 
        ConfigResetSettings, ConfigBoatType, ConfigSoundRelayId, ConfigSoundStartDelay,
        ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID, 
        ConfigWifiPassword, ConfigWifiPort, ConfigWifiState, ConfigWifiApIpAddress };
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