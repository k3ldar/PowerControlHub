#include "ConfigNetworkHandler.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "SystemDefinitions.h"


ConfigNetworkHandler::ConfigNetworkHandler(ConfigController* configController, WifiController* wifiController)
	: _configController(configController), _wifiController(wifiController)
{
}

CommandResult ConfigNetworkHandler::handleRequest(const char* method,
	const char* command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	ConfigResult result;
	if (strcmp(command, ConfigSaveSettings) == 0)
	{
		result = _configController->save();
	}
    else if (strcmp(command, ConfigResetSettings) == 0)
    {
        // Reset to defaults
        result = _configController->reset();
    }
    else if (strcmp(command, ConfigRename) == 0)
    {
        if (paramCount >= 1)
        {
            result = _configController->rename(params[0].value);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigRenameRelay) == 0)
    {
        // Expect "C4:<idx>=<shortName>" or "C4:<idx>=<shortName|longName>" where idx 0..7
        if (paramCount >= 1)
        {
            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            result = _configController->renameRelay(idx, params[0].value);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigMapHomeButton) == 0)
    {
        // Expect "MAP <button>=<relay>" where button 0..3, relay 0..7 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t relay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            result = _configController->mapHomeButton(button, relay);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigSetButtonColor) == 0)
    {
        // Expect "MAP <button>=<color>" where button 0..3, image 0..5 (or 255 to unmap)
        if (paramCount >= 1)
        {
            uint8_t button = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t buttonColor = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            result = _configController->mapHomeButtonColor(button, buttonColor);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigBoatType) == 0)
    {
        // Expect "C7:type=<value>" where value is 0..3
        if (paramCount >= 1)
        {
            uint8_t type = atoi(params[0].value);
            result = _configController->setVesselType(type);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigSoundRelayId) == 0)
    {
        // Expect "MAP <value>=<relay>" where relay 0..7 (or 255 to unmap)
        if (paramCount == 1 && strlen(params[0].value) > 0)
        {
            uint8_t relay = atoi(params[0].value);
            result = _configController->setSoundRelayButton(relay);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigSoundStartDelay) == 0)
    {
        if (paramCount == 1)
        {
            uint16_t soundStartDelay = atoi(params[0].value);
            result = _configController->setsoundDelayStart(soundStartDelay);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
#if defined(ARDUINO_UNO_R4)
    else if (strcmp(command, ConfigBluetoothEnable) == 0)
    {
        // Expect "C10:v=<0|1>"
        if (paramCount >= 1)
        {
            bool enable = SystemFunctions::parseBooleanValue(params[0].value);
            result = _configController->setBluetoothEnabled(enable);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigWifiEnable) == 0)
    {
        // Expect "C11:v=<0|1>"
        if (paramCount >= 1)
        {
            bool enable = SystemFunctions::parseBooleanValue(params[0].value);
            result = _configController->setWifiEnabled(enable);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigWifiMode) == 0)
    {
        // Expect "C12:v=<0|1>"
        if (paramCount >= 1)
        {
            uint8_t mode = static_cast<uint8_t>(atoi(params[0].value));
            result = _configController->setWifiAccessMode(mode);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigWifiSSID) == 0)
    {
        // Expect "C13:v=<value>"
        if (paramCount >= 1)
        {
            result = _configController->setWifiSsid(params[0].value);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigWifiPassword) == 0)
    {
        // Expect "C14:v=<value>"
        if (paramCount >= 1)
        {
            result = _configController->setWifiPassword(params[0].value);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else if (strcmp(command, ConfigWifiPort) == 0)
    {
        // Expect "C15:v=<value>"
        if (paramCount >= 1)
        {
            uint16_t port = static_cast<uint16_t>(atoi(params[0].value));
            result = _configController->setWifiPort(port);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
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

        snprintf(responseBuffer, sizeof(responseBuffer), "v=%d", state);
        result = ConfigResult::Success;
    }
    else if (strcmp(command, ConfigWifiApIpAddress) == 0)
    {
        // Expect "C17:v=<value>"
        if (paramCount >= 1)
        {
            result = _configController->setWifiIpAddress(params[0].value);
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
#endif
    else if (strcmp(command, ConfigDefaultRelayState) == 0)
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(atoi(params[0].key));
            result = _configController->setRelayDefaultState(relayIndex, SystemFunctions::parseBooleanValue(params[0].value));
        }
        else
        {
            result = ConfigResult::InvalidParameter;
        }
    }
    else
    {
        result = ConfigResult::InvalidCommand;
    }

	if (result == ConfigResult::Success)
    {
        return CommandResult::ok();
    }

	return CommandResult::error(static_cast<uint8_t>(result));
}

void ConfigNetworkHandler::formatStatusJson(WiFiClient* client)
{
	Config* config = ConfigManager::getConfigPtr();
	if (!config) {
		client->print("{\"error\":\"Config not available\"}");
		return;
	}

	client->print("\"config\":{");

	// Name
	client->print("\"name\":\"");
	client->print(config->name);
	client->print("\",");

	// Relays: short and long names
	client->print("\"relays\":[");
	for (uint8_t i = 0; i < ConfigRelayCount; ++i) {
		if (i > 0) client->print(",");
		client->print("{\"shortName\":\"");
		client->print(config->relayShortNames[i]);
		client->print("\",\"longName\":\"");
		client->print(config->relayLongNames[i]);
		client->print("\"}");
	}
	client->print("],");

	// Home button mappings
	client->print("\"homeButtonMapping\":[");
	for (uint8_t i = 0; i < ConfigHomeButtons; ++i) {
		if (i > 0) client->print(",");
		client->print(config->homePageMapping[i]);
	}
	client->print("],");

	// Button colors
	client->print("\"buttonColors\":[");
	for (uint8_t i = 0; i < ConfigRelayCount; ++i) {
		if (i > 0) client->print(",");
		client->print(config->buttonImage[i]);
	}
	client->print("],");

	// Vessel type
	client->print("\"vesselType\":");
	client->print(static_cast<uint8_t>(config->vesselType));
	client->print(",");

	// Sound relay ID
	client->print("\"hornRelayIndex\":");
	client->print(static_cast<uint8_t>(config->hornRelayIndex));
	client->print(",");

	// Sound start delay
	client->print("\"soundStartDelayMs\":");
	client->print(static_cast<uint16_t>(config->soundStartDelayMs));
	client->print(",");

#if defined(ARDUINO_UNO_R4)
	// Bluetooth, WiFi, SSID, Password, Port, AccessMode
	client->print("\"bluetoothEnabled\":");
	client->print(config->bluetoothEnabled ? "true" : "false");
	client->print(",");

	client->print("\"wifiEnabled\":");
	client->print(config->wifiEnabled ? "true" : "false");
	client->print(",");

	client->print("\"accessMode\":");
	client->print(config->accessMode);
	client->print(",");

	client->print("\"apSSID\":\"");
	client->print(config->apSSID);
	client->print("\",");

	client->print("\"apPassword\":\"");
	client->print(config->apPassword);
	client->print("\",");

	client->print("\"wifiPort\":");
	client->print(config->wifiPort);
	client->print(",");

	// WiFi State
	uint8_t wifiState = 0;
	if (_wifiController && _wifiController->getServer()) {
		wifiState = static_cast<uint8_t>(_wifiController->getServer()->getConnectionState());
	}
	client->print("\"wifiState\":");
	client->print(wifiState);
	client->print(",");

	// WiFi AP IP Address
	client->print("\"apIpAddress\":\"");
	if (_wifiController && _wifiController->getServer()) {
		char ipBuffer[MaxIpAddressLength];
		_wifiController->getServer()->getIpAddress(ipBuffer, sizeof(ipBuffer));
		client->print(ipBuffer);
	} else {
		client->print(config->apIpAddress);
	}
	client->print("\",");
#endif

	// Default relay states
	client->print("\"defaultRelayStates\":[");
	for (uint8_t i = 0; i < ConfigRelayCount; ++i) {
		if (i > 0) client->print(",");
		client->print(config->defaulRelayState[i] ? "true" : "false");
	}
	client->print("]");

	client->print("}");
}

void ConfigNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	formatStatusJson(client);
}
