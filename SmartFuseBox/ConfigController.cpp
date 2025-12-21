#include "ConfigController.h"
#include "SystemDefinitions.h"

#if defined(ARDUINO_UNO_R4)
// from NextionIds.h
constexpr uint8_t ImageButtonColorBlue = 2;
constexpr uint8_t ImageButtonColorYellow = 7;

#endif


ConfigController::ConfigController(SoundController* soundController,
	BluetoothController* bluetoothController, WifiController* wifiController)
	: _soundController(soundController),
	  _bluetoothController(bluetoothController),
	  _wifiController(wifiController),
	  _config(nullptr)
{
	_config = ConfigManager::getConfigPtr();
}

Config* ConfigController::getConfigPtr()
{
	return _config;
}

ConfigResult ConfigController::save()
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	return ConfigManager::save() ? ConfigResult::Success : ConfigResult::Failed;
}

ConfigResult ConfigController::reset()
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	ConfigManager::resetToDefaults();
	return ConfigResult::Success;
}

ConfigResult ConfigController::rename(const char* name)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (name == nullptr)
		return ConfigResult::InvalidParameter;

	if (strlen(name) > ConfigMaxNameLength)
		return ConfigResult::TooLong;

	strncpy(_config->name, name, sizeof(_config->name) - 1);
	_config->name[sizeof(_config->name) - 1] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::renameRelay(const uint8_t relayIndex, const char* name)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (name == nullptr)
		return ConfigResult::InvalidParameter;

	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	// Parse short and long names (format: "shortName|longName" or just "shortName")
	int pipeIndex = SystemFunctions::indexOf(name, '|', 0);
	char shortName[6] = "";
	char longName[21] = "";

	if (pipeIndex >= 0)
	{
		// Pipe character found - split into short and long names
		char tmpShortName[6] = "";
		char tmpLongName[21] = "";
		SystemFunctions::substr(tmpShortName, sizeof(tmpShortName), name, 0, pipeIndex);
		SystemFunctions::substr(tmpLongName, sizeof(tmpLongName), name, pipeIndex + 1);
		strncpy(shortName, tmpShortName, sizeof(shortName) - 1);
		strncpy(longName, tmpLongName, sizeof(longName) - 1);
	}

	// Copy short name with truncation to relay short name length
	size_t maxShortLen = sizeof(_config->relayShortNames[relayIndex]) - 1;
	strncpy(_config->relayShortNames[relayIndex], shortName, maxShortLen);
	_config->relayShortNames[relayIndex][maxShortLen] = '\0';

	// Copy long name with truncation to relay long name length
	size_t maxLongLen = sizeof(_config->relayLongNames[relayIndex]) - 1;
	strncpy(_config->relayLongNames[relayIndex], longName, maxLongLen);
	_config->relayLongNames[relayIndex][maxLongLen] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::mapHomeButton(const uint8_t homeButtonIndex, const uint8_t relayIndex)
{
	if (_config == nullptr)
		return ConfigResult::InvalidCommand;

	if (homeButtonIndex >= ConfigHomeButtons)
		return ConfigResult::InvalidParameter;
	
	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	_config->homePageMapping[homeButtonIndex] = relayIndex;
	return ConfigResult::Success;
}

ConfigResult ConfigController::mapHomeButtonColor(const uint8_t homeButtonIndex, const uint8_t colorIndex)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (homeButtonIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	uint8_t color = colorIndex;

	// Adjust to match BTN_COLOR_* constants (2..7), 255 to clear
	if (color < DefaultValue)
		color += 2;

	if ((color < ImageButtonColorBlue || color > ImageButtonColorYellow) && color != DefaultValue)
		return ConfigResult::InvalidParameter;

	_config->buttonImage[homeButtonIndex] = color;
	return ConfigResult::Success;
}

ConfigResult ConfigController::setVesselType(const uint8_t vesselType)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (vesselType > static_cast<uint8_t>(VesselType::Yacht))
		return ConfigResult::InvalidParameter;

	_config->vesselType = static_cast<VesselType>(vesselType);
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setSoundRelayButton(const uint8_t relayIndex)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount && relayIndex != DefaultValue)
		return ConfigResult::InvalidRelay;

	_config->hornRelayIndex = relayIndex;
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setsoundDelayStart(const uint16_t delayMilliSeconds)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->soundStartDelayMs = delayMilliSeconds;
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setBluetoothEnabled(const bool enabled)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->bluetoothEnabled = enabled;

	// do not apply live, only on next restart, otherwise too many enabled/disable cycles will 
	// eventually force the board to run out of memory, the only exception to this is if 
	// it starts disabled and is being enabled now, i.e. once on it needs rebooting to turn off again
	if (_bluetoothController && !_bluetoothController->isEnabled())
	{
		if (!_bluetoothController->setEnabled(enabled))
		{
			return ConfigResult::BluetoothInitFailed;
		}
	}

	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiEnabled(const bool enabled)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->wifiEnabled = enabled;

	// do not apply live, only on next restart, otherwise too many enabled/disable cycles will 
	// eventually force the board to run out of memory, the only exception to this is if 
	// it starts disabled and is being enabled now, i.e. once on it needs rebooting to turn off again
	if (_wifiController && !_wifiController->isEnabled())
	{
		if (!_wifiController->setEnabled(enabled))
		{
			return ConfigResult::WifiInitFailed;
		}
	}

	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiAccessMode(const uint8_t accessMode)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (accessMode > 1)
	{
		return ConfigResult::InvalidParameter;
	}

	_config->accessMode = accessMode;
	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiSsid(const char* ssid)
{
	if (_config == nullptr || ssid == nullptr)
		return ConfigResult::InvalidConfig;

	if (strlen(ssid) > MaxSSIDLength - 1)
		return ConfigResult::TooLong;

	strncpy(_config->apSSID, ssid, sizeof(_config->apSSID) - 1);
	_config->apSSID[sizeof(_config->apSSID) - 1] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiPassword(const char* password)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (password == nullptr)
		return ConfigResult::InvalidParameter;

	if (strlen(password) > MaxWiFiPasswordLength - 1)
		return ConfigResult::InvalidParameter;

	strncpy(_config->apPassword, password, sizeof(_config->apPassword) - 1);
	_config->apPassword[sizeof(_config->apPassword) - 1] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiPort(const uint16_t port)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (port == 0)
	{
		return ConfigResult::InvalidParameter;
	}

	_config->wifiPort = port;
	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiIpAddress(const char* ipAddress)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (ipAddress == nullptr)
		return ConfigResult::InvalidParameter;

	if (strlen(ipAddress) > MaxIpAddressLength - 1)
		return ConfigResult::TooLong;

	IPAddress ip;
	if (!ip.fromString(ipAddress))
	{
		return ConfigResult::InvalidParameter;
	}

	strncpy(_config->apIpAddress, ipAddress, sizeof(_config->apIpAddress) - 1);
	_config->apIpAddress[sizeof(_config->apIpAddress) - 1] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::setRelayDefaultState(const uint8_t relayIndex, const bool isOpen)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	_config->defaulRelayState[relayIndex] = isOpen;
	return ConfigResult::Success;
}

void ConfigController::updateSoundControllerConfig()
{
	if (_soundController != nullptr && _config != nullptr)
	{
		_soundController->configUpdated(_config);
	}
}
