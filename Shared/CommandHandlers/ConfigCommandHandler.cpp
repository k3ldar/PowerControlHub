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
#include <Arduino.h>
#include "ConfigCommandHandler.h"
#include "ConfigController.h"
#include "ConfigSyncManager.h"
#include "SystemFunctions.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardConfigLoader.h"
#endif

#if defined(MQTT_SUPPORT)
#include "MQTTConfigCommandHandler.h"
#include "MQTTController.h"
#endif


ConfigCommandHandler::ConfigCommandHandler(
	IWifiController* wifiController,
	ConfigController* configController)
	:
	_wifiController(wifiController),
	_configController(configController),
	_configSyncManager(nullptr)
#if defined(SD_CARD_SUPPORT)
	, _sdCardConfigLoader(nullptr)
#endif
#if defined(MQTT_SUPPORT)
	, _mqttConfigHandler(nullptr)
	, _mqttController(nullptr)
#endif
{}

void ConfigCommandHandler::setConfigSyncManager(ConfigSyncManager* syncManager)
{
	_configSyncManager = syncManager;
}

#if defined(MQTT_SUPPORT)
void ConfigCommandHandler::setMqttConfigHandler(MQTTConfigCommandHandler* mqttConfigHandler)
{
	_mqttConfigHandler = mqttConfigHandler;
}

void ConfigCommandHandler::setMqttController(MQTTController* mqttController)
{
	_mqttController = mqttController;
}
#endif

bool ConfigCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
	// Notify sync manager that a config command was received (if syncing)
	if (_configSyncManager)
	{
		_configSyncManager->notifyConfigReceived();
	}

	// Access the in-memory config
	ConfigResult result;

	if (strcmp(command, ConfigSaveSettings) == 0)
	{
		result = _configController->save();
	}
	else if (strcmp(command, ConfigGetSettings) == 0)
	{
		char buffer[128]{};
		buffer[0] = '\0';

		Config* config = _configController->getConfigPtr();

		if (!config)
		{
			sendAckErr(sender, command, F("Config not available"));
			return true;
		}

		// return summary of config back to caller in multiple commands
		// C3:<name>
		sender->sendCommand(ConfigRename, config->vessel.name);

		// C4 entries - send both short and long names in format: <idx>=<shortName|longName>
		for (uint8_t i = 0; i < ConfigRelayCount; ++i)
		{
			snprintf(buffer, sizeof(buffer), "%u=%s|%s", i, config->relay.shortNames[i], config->relay.longNames[i]);
			sender->sendCommand(ConfigRenameRelay, buffer);
		}

		// C5 entries
		for (uint8_t s = 0; s < ConfigHomeButtons; ++s)
		{
			snprintf(buffer, sizeof(buffer), "%u=%u", s, config->relay.homePageMapping[s]);
			sender->sendCommand(ConfigMapHomeButton, buffer);
		}

		// C6 Send home page button color mappings
		for (uint8_t i = 0; i < ConfigRelayCount; i++)
		{
			snprintf(buffer, sizeof(buffer), "%u=%u", i, config->relay.buttonImage[i]);
			sender->sendCommand(ConfigSetButtonColor, buffer);
		}

		// C7 Boat type
		snprintf(buffer, sizeof(buffer), "v=%d", static_cast<uint8_t>(config->vessel.vesselType));
		sender->sendCommand(ConfigBoatType, buffer);

		// C8 Sound relay ID
		snprintf(buffer, sizeof(buffer), "v=%d", static_cast<uint8_t>(config->sound.hornRelayIndex));
		sender->sendCommand(ConfigSoundRelayId, buffer);

		// C9 Sound start delay
		snprintf(buffer, sizeof(buffer), "v=%u", static_cast<unsigned int>(config->sound.startDelayMs));
		sender->sendCommand(ConfigSoundStartDelay, buffer);

		// C10 Bluetooth enable
		snprintf(buffer, sizeof(buffer), "v=%s", (config->network.bluetoothEnabled ? "1" : "0"));
		sender->sendCommand(ConfigBluetoothEnable, buffer);

		// C11 WiFi enable
		snprintf(buffer, sizeof(buffer), "v=%s", (config->network.wifiEnabled ? "1" : "0"));
		sender->sendCommand(ConfigWifiEnable, buffer);

		// C12 WiFi mode
		snprintf(buffer, sizeof(buffer), "v=%d", static_cast<uint8_t>(config->network.accessMode));
		sender->sendCommand(ConfigWifiMode, buffer);

		// C13 WiFi SSID
		snprintf(buffer, sizeof(buffer), "v=%s", config->network.ssid);
		sender->sendCommand(ConfigWifiSSID, buffer);

		// C14 WiFi Password
		snprintf(buffer, sizeof(buffer), "v=%s", config->network.password);
		sender->sendCommand(ConfigWifiPassword, buffer);

		// C15 WiFi Port
		snprintf(buffer, sizeof(buffer), "v=%d", config->network.port);
		sender->sendCommand(ConfigWifiPort, buffer);

		// C16 WiFi State
		if (_wifiController)
		{
			snprintf(buffer, sizeof(buffer), "v=%d", static_cast<int>(_wifiController->getConnectionState()));
			sender->sendCommand(ConfigWifiState, buffer);
		}

		// C17 WiFi AP IP Address
		if (_wifiController)
		{
			char ipBuffer[MaxIpAddressLength];
			_wifiController->getIpAddress(ipBuffer, sizeof(ipBuffer));
			snprintf(buffer, sizeof(buffer), "v=%s", ipBuffer);
			sender->sendCommand(ConfigWifiApIpAddress, buffer);
		}

		// C18 Default relay states
		for (uint8_t i = 0; i < ConfigRelayCount; ++i)
		{
			snprintf(buffer, sizeof(buffer), "%d=%s", i, (config->relay.defaultState[i] ? "1" : "0"));
			sender->sendCommand(ConfigDefaultRelayState, buffer);
		}

		// C19 Linked relays
		for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
		{
			snprintf(buffer, sizeof(buffer), "%d=%d", config->relay.linkedRelays[i][0], config->relay.linkedRelays[i][1]);
			sender->sendCommand(ConfigLinkRelays, buffer);
		}

		// C20 Timezone offset
		snprintf(buffer, sizeof(buffer), "v=%d", config->system.timezoneOffset);
		sender->sendCommand(ConfigTimeZoneOffset, buffer);

		// C21 MMSI
		sender->sendCommand(ConfigMmsi, config->vessel.mmsi);

		// C22 Call sign
		sender->sendCommand(ConfigCallSign, config->vessel.callSign);

		// C23 Home port
		sender->sendCommand(ConfigHomePort, config->vessel.homePort);

		// C24 LED Colors (send all 4: day/good, day/bad, night/good, night/bad)
		// Day mode, good color
		snprintf(buffer, sizeof(buffer), "t=0;c=0;r=%u;g=%u;b=%u",
			config->led.dayGoodColor[0],
			config->led.dayGoodColor[1],
			config->led.dayGoodColor[2]);
		sender->sendCommand(ConfigLedColor, buffer);

		// Day mode, bad color
		snprintf(buffer, sizeof(buffer), "t=0;c=1;r=%u;g=%u;b=%u",
			config->led.dayBadColor[0],
			config->led.dayBadColor[1],
			config->led.dayBadColor[2]);
		sender->sendCommand(ConfigLedColor, buffer);

		// Night mode, good color
		snprintf(buffer, sizeof(buffer), "t=1;c=0;r=%u;g=%u;b=%u",
			config->led.nightGoodColor[0],
			config->led.nightGoodColor[1],
			config->led.nightGoodColor[2]);
		sender->sendCommand(ConfigLedColor, buffer);

		// Night mode, bad color
		snprintf(buffer, sizeof(buffer), "t=1;c=1;r=%u;g=%u;b=%u",
			config->led.nightBadColor[0],
			config->led.nightBadColor[1],
			config->led.nightBadColor[2]);
		sender->sendCommand(ConfigLedColor, buffer);

		// C25 LED Brightness (send both day and night)
		snprintf(buffer, sizeof(buffer), "t=0;b=%u", config->led.dayBrightness);
		sender->sendCommand(ConfigLedBrightness, buffer);

		snprintf(buffer, sizeof(buffer), "t=1;b=%u", config->led.nightBrightness);
		sender->sendCommand(ConfigLedBrightness, buffer);

		// C26 LED Auto-switch
		snprintf(buffer, sizeof(buffer), "v=%s", config->led.autoSwitch ? "1" : "0");
		sender->sendCommand(ConfigLedAutoSwitch, buffer);

		// C27 LED Enable states
		snprintf(buffer, sizeof(buffer), "g=%s;w=%s;s=%s",
			config->led.gpsEnabled ? "1" : "0",
			config->led.warningEnabled ? "1" : "0",
			config->led.systemEnabled ? "1" : "0");
		sender->sendCommand(ConfigLedEnable, buffer);

		// C28 Control Panel Tones - Good tone
		snprintf(buffer, sizeof(buffer), "t=0;h=%u;d=%u;p=%u;r=0",
			config->sound.goodToneHz,
			config->sound.goodDurationMs,
			config->sound.goodPreset);
		sender->sendCommand(ControlPanelTones, buffer);

		// C28 Control Panel Tones - Bad tone
		snprintf(buffer, sizeof(buffer), "t=1;h=%u;d=%u;p=%u;r=%lu",
			config->sound.badToneHz,
			config->sound.badDurationMs,
			config->sound.badPreset,
			config->sound.badRepeatMs);
		sender->sendCommand(ControlPanelTones, buffer);

		// C31 SD Card Initialize Speed
		snprintf(buffer, sizeof(buffer), "v=%u", config->sdCard.initializeSpeed);
		sender->sendCommand(ConfigSdCardSpeed, buffer);

		// C32 Light sensor night relay
		snprintf(buffer, sizeof(buffer), "v=%d", config->lightSensor.nightRelayIndex);
		sender->sendCommand(ConfigLightSensorNightRelay, buffer);

		// C33 Light sensor daytime threshold
		snprintf(buffer, sizeof(buffer), "v=%u", config->lightSensor.daytimeThreshold);
		sender->sendCommand(ConfigLightSensorThreshold, buffer);

		result = ConfigResult::Success;
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
			state = static_cast<uint8_t>(_wifiController->getConnectionState());
		}

		char cmd[10];
		snprintf(cmd, sizeof(cmd), "v=%d", state);
		sender->sendCommand(ConfigWifiState, cmd);
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
		// C20 - Set timezone offset (hours from UTC, -12 to +14)
		// Format: C20:v=-5 or C20:v=8
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
		// C21 - Set MMSI (9 digits)
		// Format: C21:123456789
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
		// Format: C22:ABCD123
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
		// Format: C23:Miami
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
		// Format: C24:t=0;c=0;r=255;g=50;b=213
		// t=type (0=day, 1=night), c=colorset (0=good, 1=bad), r/g/b=0-255
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
		// Format: C25:t=0;b=75
		// t=type (0=day, 1=night), b=brightness (0-100)
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
		// Format: C26:v=true or C26:v=1
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
		// Format: C27:g=true;w=true;s=false
		// g=GPS LED, w=Warning LED, s=System LED
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
		// Format: C28:t=0;h=400;d=500;p=0;r=30000
		// t=type (0=good, 1=bad), h=tone Hz, d=duration ms, p=preset, r=repeat interval ms (bad only)
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
	else if (strcmp(command, ConfigReloadFromSd) == 0)
	{
#if defined(SD_CARD_SUPPORT)
		if (_sdCardConfigLoader)
		{
			if (_sdCardConfigLoader->reloadConfigFromSd())
			{
				result = ConfigResult::Success;
			}
			else
			{
				sendAckErr(sender, command, F("Failed to reload from SD"));
				return true;
			}
		}
		else
		{
			sendAckErr(sender, command, F("SD config loader not available"));
			return true;
		}
#else
		sendAckErr(sender, command, F("SD card not supported"));
		return true;
#endif
	}
	else if (strcmp(command, ConfigExportToSd) == 0)
	{
#if defined(SD_CARD_SUPPORT)
		if (_sdCardConfigLoader)
		{
			if (_sdCardConfigLoader->exportConfigToSd())
			{
				result = ConfigResult::Success;
			}
			else
			{
				sendAckErr(sender, command, F("Failed to export to SD"));
				return true;
			}
		}
		else
		{
			sendAckErr(sender, command, F("SD config loader not available"));
			return true;
		}
#else
		sendAckErr(sender, command, F("SD card not supported"));
		return true;
#endif
	}
	else if (strcmp(command, ConfigSdCardSpeed) == 0)
	{
#if defined(SD_CARD_SUPPORT)
		if (paramCount >= 1)
		{
			uint8_t speedMhz = static_cast<uint8_t>(atoi(params[0].value));
			result = _configController->setSdCardInitializeSpeed(speedMhz);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
#else
		sendAckErr(sender, command, F("SD card not supported"));
		return true;
#endif
	}
	else if (strcmp(command, ConfigLightSensorNightRelay) == 0)
	{
		if (paramCount == 1)
		{
			uint8_t relay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));
			result = _configController->setLightSensorNightRelay(relay);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (strcmp(command, ConfigLightSensorThreshold) == 0)
	{
		if (paramCount == 1)
		{
			uint16_t threshold = static_cast<uint16_t>(atoi(params[0].value));
			result = _configController->setLightSensorThreshold(threshold);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
#if defined(MQTT_SUPPORT)
	else if (command[0] == 'M' && strlen(command) >= 2)
	{
		// MQTT configuration commands (M0-M8)
		Config* config = _configController->getConfigPtr();

		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else if (paramCount == 0)
		{
			// Query operation - similar to C16 WiFi State
			StringKeyValue param;

			if (strcmp(command, MqttConfigEnable) == 0)
			{
				// M0 - MQTT Enabled
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%d", config->mqtt.enabled ? 1 : 0);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigBroker) == 0)
			{
				// M1 - MQTT Broker
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.broker, sizeof(param.value));
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigPort) == 0)
			{
				// M2 - MQTT Port
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%u", config->mqtt.port);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigUsername) == 0)
			{
				// M3 - MQTT Username
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.username, sizeof(param.value));
				param.value[sizeof(param.value) - 1] = '\0';
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigPassword) == 0)
			{
				// M4 - MQTT Password
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.password, sizeof(param.value));
				param.value[sizeof(param.value) - 1] = '\0';
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigDeviceId) == 0)
			{
				// M5 - MQTT Device ID
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.deviceId, sizeof(param.value));
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigHADiscovery) == 0)
			{
				// M6 - HA Discovery Enabled
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%d", config->mqtt.useHomeAssistantDiscovery ? 1 : 0);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigKeepAlive) == 0)
			{
				// M7 - Keep Alive Interval
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%u", config->mqtt.keepAliveInterval);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigState) == 0)
			{
				// M8 - MQTT Connection State
				strncpy(param.key, ValueParamName, sizeof(param.key));

				// Get actual connection state from MQTTController if available
				bool isConnected = false;
				if (_mqttController != nullptr)
				{
					isConnected = _mqttController->isConnected();
				}
				else
				{
					// Fallback to config enabled state if controller not available
					isConnected = config->mqtt.enabled;
				}

				snprintf(param.value, sizeof(param.value), "%d", isConnected ? 1 : 0);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (strcmp(command, MqttConfigDiscoveryPrefix) == 0)
			{
				// M9 - Discovery Prefix
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.discoveryPrefix, sizeof(param.value));
				param.value[sizeof(param.value) - 1] = '\0';
				sendAckOk(sender, command, &param);
				return true;
			}
			else
			{
				result = ConfigResult::InvalidCommand;
			}
		}
		else
		{
			// Set operation - delegate to MQTTConfigCommandHandler
			if (_mqttConfigHandler != nullptr)
			{
				const char* paramStr = params[0].value;

				if (_mqttConfigHandler->processCommand(command, paramStr))
				{
					result = ConfigResult::Success;
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
		}
	}
#endif
	else
	{
		result = ConfigResult::InvalidCommand;
	}

	switch (result)
	{
	case ConfigResult::Success:
		break;
	case ConfigResult::InvalidRelay:
		sendAckErr(sender, command, F("Index out of range"), &params[0]);
		return true;
	case ConfigResult::TooLong:
		sendAckErr(sender, command, F("Name too long"), &params[0]);
		return true;
	case ConfigResult::MissingName:
		sendAckErr(sender, command, F("Missing name"), &params[0]);
		return true;
	case ConfigResult::InvalidConfig:
		sendAckErr(sender, command, F("Config not available"), &params[0]);
		return true;
	case ConfigResult::InvalidCommand:
		sendAckErr(sender, command, F("Invalid command"), &params[0]);
		return true;
	case ConfigResult::InvalidParameter:
		sendAckErr(sender, command, F("Invalid parameter"), &params[0]);
		return true;
	case ConfigResult::BluetoothInitFailed:
		sendAckErr(sender, command, F("Bluetooth init failed"), &params[0]);
		return true;
	case ConfigResult::WifiInitFailed:
		sendAckErr(sender, command, F("WiFi init failed"), &params[0]);
		return true;
	default:
		sendAckErr(sender, command, F("Unknown error"), &params[0]);
		return true;
	}

	sendAckOk(sender, command, &params[0]);

	return true;
}

#if defined(SD_CARD_SUPPORT)
void ConfigCommandHandler::setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader)
{
	_sdCardConfigLoader = sdCardConfigLoader;
}
#endif

const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
	static const char* cmds[] = {
		ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings,
		ConfigRename, ConfigRenameRelay, ConfigMapHomeButton, ConfigSetButtonColor,
		ConfigBoatType, ConfigSoundRelayId, ConfigSoundStartDelay,
		ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID,
		ConfigWifiPassword, ConfigWifiPort, ConfigWifiState, ConfigWifiApIpAddress,
#if defined(MQTT_SUPPORT)
		MqttConfigEnable, MqttConfigBroker, MqttConfigPort, MqttConfigUsername,
		MqttConfigPassword, MqttConfigDeviceId, MqttConfigHADiscovery, MqttConfigKeepAlive,
		MqttConfigState, MqttConfigDiscoveryPrefix,
#endif
		ConfigDefaultRelayState, ConfigLinkRelays,
		ConfigTimeZoneOffset, ConfigMmsi, ConfigCallSign, ConfigHomePort,
		ConfigLedColor, ConfigLedBrightness, ConfigLedAutoSwitch, ConfigLedEnable,
		ControlPanelTones,
		ConfigReloadFromSd, ConfigExportToSd, ConfigSdCardSpeed,
		ConfigLightSensorNightRelay, ConfigLightSensorThreshold
	};
	count = sizeof(cmds) / sizeof(cmds[0]);
	return cmds;
}
