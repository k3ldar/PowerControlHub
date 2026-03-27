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

	strncpy(_config->vessel.name, name, sizeof(_config->vessel.name) - 1);
	_config->vessel.name[sizeof(_config->vessel.name) - 1] = '\0';
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
	size_t maxShortLen = sizeof(_config->relay.relays[relayIndex].shortName) - 1;
	strncpy(_config->relay.relays[relayIndex].shortName, shortName, maxShortLen);
	_config->relay.relays[relayIndex].shortName[maxShortLen] = '\0';

	// Copy long name with truncation to relay long name length
	size_t maxLongLen = sizeof(_config->relay.relays[relayIndex].longName) - 1;
	strncpy(_config->relay.relays[relayIndex].longName, longName, maxLongLen);
	_config->relay.relays[relayIndex].longName[maxLongLen] = '\0';
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

	_config->relay.homePageMapping[homeButtonIndex] = relayIndex;
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

	_config->relay.relays[homeButtonIndex].buttonImage = color;
	return ConfigResult::Success;
}

ConfigResult ConfigController::setVesselType(const uint8_t vesselType)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (vesselType > static_cast<uint8_t>(VesselType::Yacht))
		return ConfigResult::InvalidParameter;

	_config->vessel.vesselType = static_cast<VesselType>(vesselType);
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setSoundRelayButton(const uint8_t relayIndex)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount && relayIndex != DefaultValue)
		return ConfigResult::InvalidRelay;

	_config->sound.hornRelayIndex = relayIndex;
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setsoundDelayStart(const uint16_t delayMilliSeconds)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->sound.startDelayMs = delayMilliSeconds;
	updateSoundControllerConfig();
	return ConfigResult::Success;
}

ConfigResult ConfigController::setBluetoothEnabled(const bool enabled)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->network.bluetoothEnabled = enabled;

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

	_config->network.wifiEnabled = enabled;

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

ConfigResult ConfigController::setWifiAccessMode(const WifiMode accessMode)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	// only accept supported WiFi modes to avoid persisting invalid values from user input
	if (accessMode != WifiMode::AccessPoint && accessMode != WifiMode::Client)
		return ConfigResult::InvalidParameter;
	_config->network.accessMode = accessMode;
	return ConfigResult::Success;
}

ConfigResult ConfigController::setWifiSsid(const char* ssid)
{
	if (_config == nullptr || ssid == nullptr)
		return ConfigResult::InvalidConfig;

	if (strlen(ssid) > MaxSSIDLength - 1)
		return ConfigResult::TooLong;

	strncpy(_config->network.ssid, ssid, sizeof(_config->network.ssid) - 1);
	_config->network.ssid[sizeof(_config->network.ssid) - 1] = '\0';
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

	strncpy(_config->network.password, password, sizeof(_config->network.password) - 1);
	_config->network.password[sizeof(_config->network.password) - 1] = '\0';
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

	_config->network.port = port;
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

	strncpy(_config->network.apIpAddress, ipAddress, sizeof(_config->network.apIpAddress) - 1);
	_config->network.apIpAddress[sizeof(_config->network.apIpAddress) - 1] = '\0';
	return ConfigResult::Success;
}

ConfigResult ConfigController::setRelayDefaultState(const uint8_t relayIndex, const bool isOpen)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (relayIndex >= ConfigRelayCount)
		return ConfigResult::InvalidRelay;

	_config->relay.relays[relayIndex].defaultState = isOpen;
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

	// is relayIndex already a source in a link?
	if (_config->relay.relays[relayIndex].linkedRelay != MaxUint8Value)
		return ConfigResult::Failed;

	// is relayIndex already a target of another relay's link?
	for (uint8_t i = 0; i < ConfigRelayCount; i++)
	{
		if (_config->relay.relays[i].linkedRelay == relayIndex)
			return ConfigResult::Failed;
	}

	_config->relay.relays[relayIndex].linkedRelay = linkedRelay;
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

	// relayIndex is a source of a link
	if (_config->relay.relays[relayIndex].linkedRelay != MaxUint8Value)
	{
		_config->relay.relays[relayIndex].linkedRelay = MaxUint8Value;
		found = true;
	}

	// relayIndex is a target of someone else's link
	for (uint8_t i = 0; i < ConfigRelayCount; i++)
	{
		if (_config->relay.relays[i].linkedRelay == relayIndex)
		{
			_config->relay.relays[i].linkedRelay = MaxUint8Value;
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

	_config->system.timezoneOffset = offset;
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

	strncpy(_config->vessel.mmsi, mmsi, sizeof(_config->vessel.mmsi) - 1);
	_config->vessel.mmsi[sizeof(_config->vessel.mmsi) - 1] = '\0';
	return ConfigResult::Success;
}

// C22: Set call sign
ConfigResult ConfigController::setCallSign(const char* callSign)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (callSign == nullptr)
		return ConfigResult::InvalidParameter;

	strncpy(_config->vessel.callSign, callSign, sizeof(_config->vessel.callSign) - 1);
	_config->vessel.callSign[sizeof(_config->vessel.callSign) - 1] = '\0';
	return ConfigResult::Success;
}

// C23: Set home port
ConfigResult ConfigController::setHomePort(const char* homePort)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	if (homePort == nullptr)
		return ConfigResult::InvalidParameter;

	strncpy(_config->vessel.homePort, homePort, sizeof(_config->vessel.homePort) - 1);
	_config->vessel.homePort[sizeof(_config->vessel.homePort) - 1] = '\0';
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
			_config->led.dayGoodColor[0] = r;
			_config->led.dayGoodColor[1] = g;
			_config->led.dayGoodColor[2] = b;
		}
		else // Bad color
		{
			_config->led.dayBadColor[0] = r;
			_config->led.dayBadColor[1] = g;
			_config->led.dayBadColor[2] = b;
		}
	}
	else // Night mode
	{
		if (colorSet == 0) // Good color
		{
			_config->led.nightGoodColor[0] = r;
			_config->led.nightGoodColor[1] = g;
			_config->led.nightGoodColor[2] = b;
		}
		else // Bad color
		{
			_config->led.nightBadColor[0] = r;
			_config->led.nightBadColor[1] = g;
			_config->led.nightBadColor[2] = b;
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
		_config->led.dayBrightness = brightness;
	else
		_config->led.nightBrightness = brightness;

	return ConfigResult::Success;
}

// C26: Set LED auto-switch
ConfigResult ConfigController::setLedAutoSwitch(const bool enabled)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->led.autoSwitch = enabled;
	return ConfigResult::Success;
}

// C27: Set LED enable states
ConfigResult ConfigController::setLedEnableStates(const bool gps, const bool warning, const bool system)
{
	if (_config == nullptr)
		return ConfigResult::InvalidConfig;

	_config->led.gpsEnabled = gps;
	_config->led.warningEnabled = warning;
	_config->led.systemEnabled = system;

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
		_config->sound.goodPreset = preset;
		_config->sound.goodToneHz = toneHz;
		_config->sound.goodDurationMs = durationMs;
		// repeat is only for bad sounds, ignore for good
	}
	else // Bad tone
	{
		_config->sound.badPreset = preset;
		_config->sound.badToneHz = toneHz;
		_config->sound.badDurationMs = durationMs;
		_config->sound.badRepeatMs = repeatMs;
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

	_config->sdCard.initializeSpeed = speedMhz;
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
