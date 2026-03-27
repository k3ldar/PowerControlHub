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
#include "Local.h"
#include "ConfigNetworkHandler.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"

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
			WifiMode mode = static_cast<WifiMode>(atoi(params[0].value));
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

		int len = snprintf(responseBuffer, bufferSize, "v=%d", state);
		result = (len > 0 && len < static_cast<int>(bufferSize)) ? ConfigResult::Success : ConfigResult::InvalidParameter;
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
	else if (strcmp(command, ConfigLinkRelays) == 0)
	{
		// Expect "C19:<relay1>=<relay2>" to link relay1 to relay2
		// or "C19:<relay1>=0xFF" to unlink relay1
		if (paramCount >= 1)
		{
			uint8_t relay1 = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
			uint8_t relay2 = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

			if (relay2 < MaxUint8Value)
			{
				result = _configController->linkRelays(relay1, relay2);
			}
			else
			{
				result = _configController->unlinkRelay(relay1);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigTimeZoneOffset) == 0)
	{
		// C20 - Set timezone offset
		if (paramCount >= 1)
		{
			int8_t offset = static_cast<int8_t>(atoi(params[0].value));
			result = _configController->setTimezoneOffset(offset);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigMmsi) == 0)
	{
		// C21 - Set MMSI
		if (paramCount >= 1)
		{
			result = _configController->setMmsi(params[0].value);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigCallSign) == 0)
	{
		// C22 - Set call sign
		if (paramCount >= 1)
		{
			result = _configController->setCallSign(params[0].value);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigHomePort) == 0)
	{
		// C23 - Set home port
		if (paramCount >= 1)
		{
			result = _configController->setHomePort(params[0].value);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigLedColor) == 0)
	{
		// C24 - Set LED RGB color
		if (paramCount >= 5)
		{
			uint8_t type = 0, colorSet = 0, r = 0, g = 0, b = 0;

			for (uint8_t i = 0; i < paramCount; i++)
			{
				if (strcmp(params[i].key, "t") == 0)
					type = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "c") == 0)
					colorSet = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "r") == 0)
					r = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "g") == 0)
					g = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "b") == 0)
					b = static_cast<uint8_t>(atoi(params[i].value));
			}

			result = _configController->setLedColor(type, colorSet, r, g, b);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigLedBrightness) == 0)
	{
		// C25 - Set LED brightness
		if (paramCount >= 2)
		{
			uint8_t type = 0, brightness = 0;

			for (uint8_t i = 0; i < paramCount; i++)
			{
				if (strcmp(params[i].key, "t") == 0)
					type = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "b") == 0)
					brightness = static_cast<uint8_t>(atoi(params[i].value));
			}

			result = _configController->setLedBrightness(type, brightness);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigLedAutoSwitch) == 0)
	{
		// C26 - Enable/disable auto day/night switching
		if (paramCount >= 1)
		{
			bool enabled = SystemFunctions::parseBooleanValue(params[0].value);
			result = _configController->setLedAutoSwitch(enabled);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigLedEnable) == 0)
	{
		// C27 - Enable/disable individual LEDs
		if (paramCount >= 3)
		{
			bool gps = false, warning = false, system = false;

			for (uint8_t i = 0; i < paramCount; i++)
			{
				if (strcmp(params[i].key, "g") == 0)
					gps = SystemFunctions::parseBooleanValue(params[i].value);
				else if (strcmp(params[i].key, "w") == 0)
					warning = SystemFunctions::parseBooleanValue(params[i].value);
				else if (strcmp(params[i].key, "s") == 0)
					system = SystemFunctions::parseBooleanValue(params[i].value);
			}

			result = _configController->setLedEnableStates(gps, warning, system);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ControlPanelTones) == 0)
	{
		// C28 - Configure control panel tones
		if (paramCount >= 4)
		{
			uint8_t type = 0, preset = 0;
			uint16_t toneHz = 0, durationMs = 0;
			uint32_t repeatMs = 0;

			for (uint8_t i = 0; i < paramCount; i++)
			{
				if (strcmp(params[i].key, "t") == 0)
					type = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "h") == 0)
					toneHz = static_cast<uint16_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "d") == 0)
					durationMs = static_cast<uint16_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "p") == 0)
					preset = static_cast<uint8_t>(atoi(params[i].value));
				else if (strcmp(params[i].key, "r") == 0)
					repeatMs = static_cast<uint32_t>(strtoul(params[i].value, nullptr, 0));
			}

			result = _configController->setControlPanelTones(type, preset, toneHz, durationMs, repeatMs);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigSdCardSpeed) == 0)
	{
		// C31 - Set SD card initialize speed
		// Format: C31:v=4 (or 8, 12, 16, 20, 24)
		if (paramCount >= 1)
		{
			uint8_t speedMhz = static_cast<uint8_t>(atoi(params[0].value));
			result = _configController->setSdCardInitializeSpeed(speedMhz);
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

void ConfigNetworkHandler::formatStatusJson(IWifiClient* client)
{
	Config* config = ConfigManager::getConfigPtr();

	if (!config)
	{
		client->print("{\"error\":\"Config not available\"}");
		return;
	}

	client->print("\"config\":{");

	// Name
	client->print("\"name\":\"");
	client->print(config->vessel.name);
	client->print("\",");

	// Relays: short and long names
	client->print("\"relays\":[");

	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		if (i > 0) client->print(",");
		client->print("{\"shortName\":\"");
		client->print(config->relay.relays[i].shortName);
		client->print("\",\"longName\":\"");
		client->print(config->relay.relays[i].longName);
		client->print("\"}");
	}

	client->print("],");

	// Home button mappings
	client->print("\"homeButtonMapping\":[");

	for (uint8_t i = 0; i < ConfigHomeButtons; ++i)
	{
		if (i > 0)
			client->print(",");

		client->print(config->relay.homePageMapping[i]);
	}

	client->print("],");

	// Button colors
	client->print("\"buttonColors\":[");

	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		if (i > 0)
			client->print(",");

		client->print(config->relay.relays[i].buttonImage);
	}

	client->print("],");

	// Vessel type
	client->print("\"vesselType\":");
	client->print(static_cast<uint8_t>(config->vessel.vesselType));
	client->print(",");

	// Sound relay ID
	client->print("\"hornRelayIndex\":");
	client->print(static_cast<uint8_t>(config->sound.hornRelayIndex));
	client->print(",");

	// Sound start delay
	client->print("\"soundStartDelayMs\":");
	client->print(static_cast<uint16_t>(config->sound.startDelayMs));
	client->print(",");

	// Bluetooth, WiFi, SSID, Password, Port, AccessMode
	client->print("\"bluetoothEnabled\":");
	client->print(config->network.bluetoothEnabled ? "true" : "false");
	client->print(",");

	client->print("\"wifiEnabled\":");
	client->print(config->network.wifiEnabled ? "true" : "false");
	client->print(",");

	client->print("\"accessMode\":");
	client->print(static_cast<uint8_t>(config->network.accessMode));
	client->print(",");

	client->print("\"apSSID\":\"");
	client->print(config->network.ssid);
	client->print("\",");

	client->print("\"apPassword\":\"");
	client->print(config->network.password);
	client->print("\",");

	client->print("\"wifiPort\":");
	client->print(config->network.port);
	client->print(",");

	// WiFi State
	uint8_t wifiState = 0;
	if (_wifiController && _wifiController->getServer())
	{
		wifiState = static_cast<uint8_t>(_wifiController->getServer()->getConnectionState());
	}

	client->print("\"wifiState\":");
	client->print(wifiState);
	client->print(",");

	// WiFi AP IP Address
	client->print("\"apIpAddress\":\"");

	if (_wifiController && _wifiController->getServer())
	{
		char ipBuffer[MaxIpAddressLength];
		_wifiController->getServer()->getIpAddress(ipBuffer, sizeof(ipBuffer));
		client->print(ipBuffer);
	}
	else
	{
		client->print(config->network.apIpAddress);
	}

	client->print("\",");

	// Default relay states
	client->print("\"defaultRelayStates\":[");

	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		if (i > 0)
			client->print(",");

		client->print(config->relay.relays[i].defaultState ? "true" : "false");
	}

	client->print("],");
	client->print("\"linkedRelays\":[");

	for (uint8_t i = 0; i < ConfigRelayCount; ++i)
	{
		if (i > 0)
			client->print(",");

		client->print("[");
		client->print(i);
		client->print(",");
		client->print(config->relay.relays[i].linkedRelay);
		client->print("]");
	}
	client->print("],");

	// C20 Timezone offset
	client->print("\"timezoneOffset\":");
	client->print(static_cast<int>(config->system.timezoneOffset));
	client->print(",");

	// C21 MMSI
	client->print("\"mmsi\":\"");
	client->print(config->vessel.mmsi);
	client->print("\",");

	// C22 Call sign
	client->print("\"callSign\":\"");
	client->print(config->vessel.callSign);
	client->print("\",");

	// C23 Home port
	client->print("\"homePort\":\"");
	client->print(config->vessel.homePort);
	client->print("\",");

	// C24 LED Colors
	client->print("\"ledColors\":{");
	client->print("\"dayGood\":[");
	client->print(config->led.dayGoodColor[0]);
	client->print(",");
	client->print(config->led.dayGoodColor[1]);
	client->print(",");
	client->print(config->led.dayGoodColor[2]);
	client->print("],");

	client->print("\"dayBad\":[");
	client->print(config->led.dayBadColor[0]);
	client->print(",");
	client->print(config->led.dayBadColor[1]);
	client->print(",");
	client->print(config->led.dayBadColor[2]);
	client->print("],");

	client->print("\"nightGood\":[");
	client->print(config->led.nightGoodColor[0]);
	client->print(",");
	client->print(config->led.nightGoodColor[1]);
	client->print(",");
	client->print(config->led.nightGoodColor[2]);
	client->print("],");

	client->print("\"nightBad\":[");
	client->print(config->led.nightBadColor[0]);
	client->print(",");
	client->print(config->led.nightBadColor[1]);
	client->print(",");
	client->print(config->led.nightBadColor[2]);
	client->print("]");
	client->print("},");

	// C25 LED Brightness
	client->print("\"ledBrightness\":{");
	client->print("\"day\":");
	client->print(config->led.dayBrightness);
	client->print(",\"night\":");
	client->print(config->led.nightBrightness);
	client->print("},");

	// C26 LED Auto-switch
	client->print("\"ledAutoSwitch\":");
	client->print(config->led.autoSwitch ? "true" : "false");
	client->print(",");

	// C27 LED Enable states
	client->print("\"ledEnable\":{");
	client->print("\"gps\":");
	client->print(config->led.gpsEnabled ? "true" : "false");
	client->print(",\"warning\":");
	client->print(config->led.warningEnabled ? "true" : "false");
	client->print(",\"system\":");
	client->print(config->led.systemEnabled ? "true" : "false");
	client->print("},");

	// C28 Control Panel Tones
	client->print("\"soundConfig\":{");
	client->print("\"goodPreset\":");
	client->print(config->sound.goodPreset);
	client->print(",\"goodToneHz\":");
	client->print(config->sound.goodToneHz);
	client->print(",\"goodDurationMs\":");
	client->print(config->sound.goodDurationMs);
	client->print(",\"badPreset\":");
	client->print(config->sound.badPreset);
	client->print(",\"badToneHz\":");
	client->print(config->sound.badToneHz);
	client->print(",\"badDurationMs\":");
	client->print(config->sound.badDurationMs);
	client->print(",\"badRepeatMs\":");
	client->print(config->sound.badRepeatMs);
	client->print("},");

	// C31 SD Card Initialize Speed
	client->print("\"sdCardInitializeSpeed\":");
	client->print(config->sdCard.initializeSpeed);

	client->print("}");
}

void ConfigNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	formatStatusJson(client);
}
