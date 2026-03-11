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
#pragma once

#include "Local.h"
#include "ConfigManager.h"
#include "SoundController.h"
#include "RelayCommandHandler.h"

// Forward declarations
class IBluetoothRadio;
class IWifiController;

enum class ConfigResult : uint8_t
{
	Success = 0,
	InvalidConfig = 1,
	Failed = 2,
	MissingName = 3,
	TooLong = 4,
	InvalidRelay = 5,
	InvalidCommand = 6,
	InvalidParameter = 7,
	BluetoothInitFailed = 8,
	WifiInitFailed = 9
};

class ConfigController
{
private:
	SoundController* _soundController;
	IBluetoothRadio* _bluetoothRadio;
	IWifiController* _wifiController;
	RelayController* _relayController;
	Config* _config;

	void updateSoundControllerConfig(); 
public:

	ConfigController(SoundController* soundController,
		IBluetoothRadio* bluetoothRadio,
		IWifiController* wifiController,
		RelayController* relayController);

	Config* getConfigPtr();
	ConfigResult save();
	ConfigResult reset();
	ConfigResult rename(const char* name);
	ConfigResult renameRelay(const uint8_t relayIndex, const char* name);
	ConfigResult mapHomeButton(const uint8_t homeButtonIndex, const uint8_t relayIndex);
	ConfigResult mapHomeButtonColor(const uint8_t homeButtonIndex, const uint8_t colorIndex);
	ConfigResult setVesselType(const uint8_t vesselType);
	ConfigResult setSoundRelayButton(const uint8_t relayIndex);
	ConfigResult setsoundDelayStart(const uint16_t delayMilliSeconds);
	ConfigResult setBluetoothEnabled(const bool enabled);
	ConfigResult setWifiEnabled(const bool enabled);
	ConfigResult setWifiAccessMode(const uint8_t accessMode);
	ConfigResult setWifiSsid(const char* ssid);
	ConfigResult setWifiPassword(const char* password);
	ConfigResult setWifiPort(const uint16_t port);
	ConfigResult setWifiIpAddress(const char* ipAddress);
	ConfigResult setRelayDefaultState(const uint8_t relayIndex, const bool isOpen);
	ConfigResult linkRelays(uint8_t relayIndex, uint8_t linkedRelay);
	ConfigResult unlinkRelay(uint8_t relayIndex);
	ConfigResult setTimezoneOffset(const int8_t offset);
	ConfigResult setMmsi(const char* mmsi);
	ConfigResult setCallSign(const char* callSign);
	ConfigResult setHomePort(const char* homePort);
	ConfigResult setLedColor(const uint8_t type, const uint8_t colorSet, const uint8_t r, const uint8_t g, const uint8_t b);
	ConfigResult setLedBrightness(const uint8_t type, const uint8_t brightness);
	ConfigResult setLedAutoSwitch(const bool enabled);
	ConfigResult setLedEnableStates(const bool gps, const bool warning, const bool system);
	ConfigResult setControlPanelTones(const uint8_t type, const uint8_t preset, const uint16_t toneHz, const uint16_t durationMs, const uint32_t repeatMs);
	ConfigResult setSdCardInitializeSpeed(const uint8_t speedMhz);
	ConfigResult setLightSensorNightRelay(const uint8_t relayIndex);
	ConfigResult setLightSensorThreshold(const uint16_t threshold);
};