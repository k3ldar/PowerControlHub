#pragma once


#include "ConfigManager.h"
#include "BluetoothController.h"
#include "SoundController.h"
#include "RelayCommandHandler.h"
#include "WifiController.h"

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
	BluetoothController* _bluetoothController;
	WifiController* _wifiController;
	RelayController* _relayController;
	Config* _config;

	void updateSoundControllerConfig(); 
public:
	ConfigController(SoundController* soundController,
		BluetoothController* bluetoothController, WifiController* wifiController,
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
};