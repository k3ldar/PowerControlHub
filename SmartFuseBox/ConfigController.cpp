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
#include "ConfigController.h"
#include "SystemDefinitions.h"
#include "RelayController.h"
#include "IBluetoothRadio.h"
#include "IWifiController.h"
#include "SystemFunctions.h"

#if defined(FUSE_BOX_CONTROLLER)
// from NextionIds.h
constexpr uint8_t ImageButtonColorBlue = 2;
constexpr uint8_t ImageButtonColorYellow = 7;

#endif


ConfigController::ConfigController(SoundController* soundController, 
	IBluetoothRadio* bluetoothRadio,
	IWifiController* wifiController,
	RelayController* relayController)
	: _soundController(soundController),
	_bluetoothRadio(bluetoothRadio),
	_wifiController(wifiController),
	_relayController(relayController),
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
	if (_bluetoothRadio && !_bluetoothRadio->isEnabled())
	{
		if (!_bluetoothRadio->setEnabled(enabled))
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

ConfigResult ConfigController::linkRelays(uint8_t relayIndex, uint8_t linkedRelay)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	if (linkedRelay >= ConfigRelayCount && linkedRelay != MaxUint8Value)
		return ConfigResult::InvalidParameter;

	if (relayIndex == linkedRelay)
		return ConfigResult::InvalidParameter;

	if (relayIndex == _relayController->getReservedSoundRelay() ||
		linkedRelay == _relayController->getReservedSoundRelay())
	{
		// cannot link sound relay
		return ConfigResult::InvalidParameter;
	}

	uint8_t availableIndex = MaxUint8Value;

	// is the relay already linked?
	for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
	{
		if (_config->linkedRelays[i][0] == relayIndex || _config->linkedRelays[i][1] == relayIndex)
		{
			return ConfigResult::Failed;
		}
	}

	// find next linked relay space
	for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
	{
		if (_config->linkedRelays[i][0] == MaxUint8Value)
		{
			availableIndex = i;
			break;
		}
	}

	if (availableIndex == MaxUint8Value)
		return ConfigResult::Failed;

	_config->linkedRelays[availableIndex][0] = relayIndex;
	_config->linkedRelays[availableIndex][1] = linkedRelay;
	return ConfigResult::Success;
}


ConfigResult ConfigController::unlinkRelay(uint8_t relayIndex)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	// find next linked relay space
	bool found = false;
	for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
	{
		if (_config->linkedRelays[i][0] == relayIndex || _config->linkedRelays[i][1] == relayIndex)
		{
			_config->linkedRelays[i][0] = MaxUint8Value;
			_config->linkedRelays[i][1] = MaxUint8Value;
			found = true;
		}
	}

	return found ? ConfigResult::Success : ConfigResult::Failed;
}

// C20: Set timezone offset
ConfigResult ConfigController::setTimezoneOffset(const int8_t offset)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// Validate range: -12 to +14 hours
	if (offset < -12 || offset > 14)
		return ConfigResult::InvalidParameter;

	_config->timezoneOffset = offset;
	return ConfigResult::Success;
}

// C21: Set MMSI
ConfigResult ConfigController::setMmsi(const char* mmsi)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (mmsi == nullptr)
		return ConfigResult::InvalidParameter;

	// Validate MMSI is exactly 9 digits
	size_t len = strlen(mmsi);
	if (len != 9)
		return ConfigResult::InvalidParameter;

	for (size_t i = 0; i < len; i++)
	{
		if (!isdigit(mmsi[i]))
			return ConfigResult::InvalidParameter;
	}

	strncpy(_config->mMSI, mmsi, sizeof(_config->mMSI) - 1);
	_config->mMSI[sizeof(_config->mMSI) - 1] = '\0';
	return ConfigResult::Success;
}

// C22: Set call sign
ConfigResult ConfigController::setCallSign(const char* callSign)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (callSign == nullptr)
		return ConfigResult::InvalidParameter;

	strncpy(_config->callSign, callSign, sizeof(_config->callSign) - 1);
	_config->callSign[sizeof(_config->callSign) - 1] = '\0';
	return ConfigResult::Success;
}

// C23: Set home port
ConfigResult ConfigController::setHomePort(const char* homePort)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (homePort == nullptr)
		return ConfigResult::InvalidParameter;

	strncpy(_config->homePort, homePort, sizeof(_config->homePort) - 1);
	_config->homePort[sizeof(_config->homePort) - 1] = '\0';
	return ConfigResult::Success;
}

// C24: Set LED RGB color
ConfigResult ConfigController::setLedColor(const uint8_t type, const uint8_t colorSet, 
										   const uint8_t r, const uint8_t g, const uint8_t b)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// type: 0=day, 1=night
	// colorSet: 0=good, 1=bad
	if (type > 1 || colorSet > 1)
		return ConfigResult::InvalidParameter;

	if (type == 0) // Day mode
	{
		if (colorSet == 0) // Good color
		{
			_config->ledConfig.dayGoodColor[0] = r;
			_config->ledConfig.dayGoodColor[1] = g;
			_config->ledConfig.dayGoodColor[2] = b;
		}
		else // Bad color
		{
			_config->ledConfig.dayBadColor[0] = r;
			_config->ledConfig.dayBadColor[1] = g;
			_config->ledConfig.dayBadColor[2] = b;
		}
	}
	else // Night mode
	{
		if (colorSet == 0) // Good color
		{
			_config->ledConfig.nightGoodColor[0] = r;
			_config->ledConfig.nightGoodColor[1] = g;
			_config->ledConfig.nightGoodColor[2] = b;
		}
		else // Bad color
		{
			_config->ledConfig.nightBadColor[0] = r;
			_config->ledConfig.nightBadColor[1] = g;
			_config->ledConfig.nightBadColor[2] = b;
		}
	}

	return ConfigResult::Success;
}

// C25: Set LED brightness
ConfigResult ConfigController::setLedBrightness(const uint8_t type, const uint8_t brightness)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// type: 0=day, 1=night
	// brightness: 0-100
	if (type > 1 || brightness > 100)
		return ConfigResult::InvalidParameter;

	if (type == 0)
		_config->ledConfig.dayBrightness = brightness;
	else
		_config->ledConfig.nightBrightness = brightness;

	return ConfigResult::Success;
}

// C26: Set LED auto-switch
ConfigResult ConfigController::setLedAutoSwitch(const bool enabled)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->ledConfig.autoSwitch = enabled;
	return ConfigResult::Success;
}

// C27: Set LED enable states
ConfigResult ConfigController::setLedEnableStates(const bool gps, const bool warning, const bool system)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->ledConfig.gpsEnabled = gps;
	_config->ledConfig.warningEnabled = warning;
	_config->ledConfig.systemEnabled = system;

	return ConfigResult::Success;
}

// C28: Set control panel tones
ConfigResult ConfigController::setControlPanelTones(const uint8_t type, const uint8_t preset, 
													const uint16_t toneHz, const uint16_t durationMs, 
													const uint32_t repeatMs)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// type: 0=good, 1=bad
	if (type > 1)
		return ConfigResult::InvalidParameter;

	// preset: 0=custom, 1=submarine ping, 2=double beep, 3=rising chirp, 
	//         4=descending alert, 5=nautical bell, 0xFFFF=no sound
	// For preset validation, we allow 0-5 or 0xFFFF (65535)
	if (preset > 5 && preset != 0xFF)
		return ConfigResult::InvalidParameter;

	if (type == 0) // Good tone
	{
		_config->soundConfig.goodPreset = preset;
		_config->soundConfig.good_toneHz = toneHz;
		_config->soundConfig.good_durationMs = durationMs;
		// repeat is only for bad sounds, ignore for good
	}
	else // Bad tone
	{
		_config->soundConfig.badPreset = preset;
		_config->soundConfig.bad_toneHz = toneHz;
		_config->soundConfig.bad_durationMs = durationMs;
		_config->soundConfig.bad_repeatMs = repeatMs;
	}

	return ConfigResult::Success;
}

// C31: Set SD card initialize speed
ConfigResult ConfigController::setSdCardInitializeSpeed(const uint8_t speedMhz)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// Only allow specific valid speeds: 4, 8, 12, 16, 20, 24 MHz
	if (speedMhz != 4 && speedMhz != 8 && speedMhz != 12 && 
		speedMhz != 16 && speedMhz != 20 && speedMhz != 24)
		return ConfigResult::InvalidParameter;

	_config->sdCardInitializeSpeed = speedMhz;
	return ConfigResult::Success;
}

// C32: Set light sensor night relay
ConfigResult ConfigController::setLightSensorNightRelay(const uint8_t relayIndex)
{
    if (_config == nullptr)
        return ConfigResult::InvalidConfig;

    if (relayIndex >= ConfigRelayCount && relayIndex != DefaultValue)
        return ConfigResult::InvalidRelay;

    _config->lightSensor.nightRelayIndex = relayIndex;
    return ConfigResult::Success;
}

// C33: Set light sensor daytime threshold
ConfigResult ConfigController::setLightSensorThreshold(const uint16_t threshold)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (threshold > 1023)
		return ConfigResult::InvalidParameter;

	_config->lightSensor.daytimeThreshold = threshold;
	return ConfigResult::Success;
}
