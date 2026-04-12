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
#include "SystemFunctions.h"

#if defined(CARD_CONFIG_LOADER)
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
	_configController(configController)
#if defined(CARD_CONFIG_LOADER)
	, _sdCardConfigLoader(nullptr)
#endif
#if defined(MQTT_SUPPORT)
	, _mqttConfigHandler(nullptr)
	, _mqttController(nullptr)
#endif
{}

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
	// Access the in-memory config
	ConfigResult result;

	if (SystemFunctions::commandMatches(command, ConfigSaveSettings))
	{
		result = _configController->save();
	}
	else if (SystemFunctions::commandMatches(command, ConfigGetSettings))
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
		sender->sendCommand(ConfigRename, config->location.name);

		// C4 SPI pins
		snprintf(buffer, sizeof(buffer), "sck=%u;mosi=%u;miso=%u",
			config->spiPins.sckPin,
			config->spiPins.mosiPin,
			config->spiPins.misoPin);
		sender->sendCommand(ConfigSpiPins, buffer);

		// C5 entries
		for (uint8_t s = 0; s < ConfigHomeButtons; ++s)
		{
			snprintf(buffer, sizeof(buffer), "%u=%u", s, config->relay.homePageMapping[s]);
			sender->sendCommand(ConfigMapHomeButton, buffer);
		}

		// C7 Boat type
		snprintf(buffer, sizeof(buffer), "v=%d", static_cast<uint8_t>(config->location.locationType));
		sender->sendCommand(ConfigBoatType, buffer);

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

		// C20 Timezone offset
		snprintf(buffer, sizeof(buffer), "v=%d", config->system.timezoneOffset);
		sender->sendCommand(ConfigTimeZoneOffset, buffer);

		// C21 MMSI
		sender->sendCommand(ConfigMmsi, config->location.mmsi);

		// C22 Call sign
		sender->sendCommand(ConfigCallSign, config->location.callSign);

		// C23 Home port
		sender->sendCommand(ConfigHomePort, config->location.homePort);

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

		// C32 SD Card CS pin
		snprintf(buffer, sizeof(buffer), "v=%u", config->sdCard.csPin);
		sender->sendCommand(ConfigSdCardCsPin, buffer);

		// C33 Light sensor daytime threshold
		snprintf(buffer, sizeof(buffer), "v=%u", config->lightSensor.daytimeThreshold);
		sender->sendCommand(ConfigLightSensorThreshold, buffer);

		result = ConfigResult::Success;
	}
	else if (SystemFunctions::commandMatches(command, ConfigResetSettings))
	{
		// Reset to defaults
		result = _configController->reset();
	}
	else if (SystemFunctions::commandMatches(command, ConfigRename))
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
	else if (SystemFunctions::commandMatches(command, ConfigMapHomeButton))
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
  else if (SystemFunctions::commandMatches(command, ConfigSpiPins))
	{
		if (paramCount >= 3)
		{
			uint8_t sckPin, mosiPin, misoPin;
			if (!getParamValueU8t(params, paramCount, "sck", sckPin) ||
				!getParamValueU8t(params, paramCount, "mosi", mosiPin) ||
				!getParamValueU8t(params, paramCount, "miso", misoPin))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setSpiPins(sckPin, mosiPin, misoPin);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigBoatType))
	{
		// Expect "C7:type=<value>" where value is 0..3
		if (paramCount >= 1)
		{
			uint8_t type = atoi(params[0].value);
			result = _configController->setLocationType(type);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigSoundStartDelay))
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
	else if (SystemFunctions::commandMatches(command, ConfigBluetoothEnable))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiEnable))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiMode))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiSSID))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiPassword))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiPort))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiState))
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
	else if (SystemFunctions::commandMatches(command, ConfigWifiApIpAddress))
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
	else if (SystemFunctions::commandMatches(command, ConfigTimeZoneOffset))
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
	else if (SystemFunctions::commandMatches(command, ConfigMmsi))
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
	else if (SystemFunctions::commandMatches(command, ConfigCallSign))
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
	else if (SystemFunctions::commandMatches(command, ConfigHomePort))
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
	else if (SystemFunctions::commandMatches(command, ConfigLedColor))
	{
		// C24 - Set LED RGB color
		// Format: C24:t=0;c=0;r=255;g=50;b=213
		// t=type (0=day, 1=night), c=colorset (0=good, 1=bad), r/g/b=0-255
		if (paramCount >= 5)
		{
			uint8_t type, colorSet, r, g, b;
			if (!getParamValueU8t(params, paramCount, "t", type) ||
				!getParamValueU8t(params, paramCount, "c", colorSet) ||
				!getParamValueU8t(params, paramCount, "r", r) ||
				!getParamValueU8t(params, paramCount, "g", g) ||
				!getParamValueU8t(params, paramCount, "b", b))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setLedColor(type, colorSet, r, g, b);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigLedBrightness))
	{
		// C25 - Set LED brightness
		// Format: C25:t=0;b=75
		// t=type (0=day, 1=night), b=brightness (0-100)
		if (paramCount >= 2)
		{
			uint8_t type, brightness;
			if (!getParamValueU8t(params, paramCount, "t", type) ||
				!getParamValueU8t(params, paramCount, "b", brightness))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setLedBrightness(type, brightness);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigLedAutoSwitch))
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
	else if (SystemFunctions::commandMatches(command, ConfigLedEnable))
	{
		// C27 - Enable/disable individual LEDs
		// Format: C27:g=true;w=true;s=false
		// g=GPS LED, w=Warning LED, s=System LED
		if (paramCount >= 3)
		{
			bool gps, warning, system;

			if (!getParamValueBool(params, paramCount, "g", gps) ||
				!getParamValueBool(params, paramCount, "w", warning) ||
				!getParamValueBool(params, paramCount, "s", system))
			{
				result = ConfigResult::InvalidParameter;
			}

			result = _configController->setLedEnableStates(gps, warning, system);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ControlPanelTones))
	{
		// C28 - Configure control panel tones
		// Format: C28:t=0;h=400;d=500;p=0;r=30000
		// t=type (0=good, 1=bad), h=tone Hz, d=duration ms, p=preset, r=repeat interval ms (bad only)
		if (paramCount >= 4)
		{
			uint8_t type, preset;
			uint16_t toneHz, durationMs;
			uint32_t repeatMs;
			if (!getParamValueU8t(params, paramCount, "t", type) ||
				!getParamValueU8t(params, paramCount, "p", preset) ||
				!getParamValueU16t(params, paramCount, "h", toneHz) ||
				!getParamValueU16t(params, paramCount, "d", durationMs) ||
				!getParamValueU32t(params, paramCount, "r", repeatMs))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setControlPanelTones(type, preset, toneHz, durationMs, repeatMs);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigReloadFromSd))
	{
#if defined(CARD_CONFIG_LOADER)
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
	else if (SystemFunctions::commandMatches(command, ConfigExportToSd))
	{
#if defined(CARD_CONFIG_LOADER)
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
	else if (SystemFunctions::commandMatches(command, ConfigSdCardSpeed))
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
	else if (SystemFunctions::commandMatches(command, ConfigSdCardCsPin))
	{
#if defined(SD_CARD_SUPPORT)
		if (paramCount >= 1)
		{
			uint8_t csPin = static_cast<uint8_t>(atoi(params[0].value));
			result = _configController->setSdCardCsPin(csPin);
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
	else if (SystemFunctions::commandMatches(command, ConfigLightSensorThreshold))
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

			if (SystemFunctions::commandMatches(command, MqttConfigEnable))
			{
				// M0 - MQTT Enabled
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%d", config->mqtt.enabled ? 1 : 0);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigBroker))
			{
				// M1 - MQTT Broker
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.broker, sizeof(param.value));
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigPort))
			{
				// M2 - MQTT Port
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%u", config->mqtt.port);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigUsername))
			{
				// M3 - MQTT Username
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.username, sizeof(param.value));
				param.value[sizeof(param.value) - 1] = '\0';
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigPassword))
			{
				// M4 - MQTT Password
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.password, sizeof(param.value));
				param.value[sizeof(param.value) - 1] = '\0';
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigDeviceId))
			{
				// M5 - MQTT Device ID
				strncpy(param.key, ValueParamName, sizeof(param.key));
				strncpy(param.value, config->mqtt.deviceId, sizeof(param.value));
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigHADiscovery))
			{
				// M6 - HA Discovery Enabled
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%d", config->mqtt.useHomeAssistantDiscovery ? 1 : 0);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigKeepAlive))
			{
				// M7 - Keep Alive Interval
				strncpy(param.key, ValueParamName, sizeof(param.key));
				snprintf(param.value, sizeof(param.value), "%u", config->mqtt.keepAliveInterval);
				sendAckOk(sender, command, &param);
				return true;
			}
			else if (SystemFunctions::commandMatches(command, MqttConfigState))
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
			else if (SystemFunctions::commandMatches(command, MqttConfigDiscoveryPrefix))
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

#if defined(CARD_CONFIG_LOADER)
void ConfigCommandHandler::setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader)
{
	_sdCardConfigLoader = sdCardConfigLoader;
}
#endif

const char* const* ConfigCommandHandler::supportedCommands(size_t& count) const
{
	static const char* cmds[] = {
		ConfigSaveSettings, ConfigGetSettings, ConfigResetSettings,
		ConfigRename, ConfigMapHomeButton,
      ConfigSpiPins,
		ConfigBoatType, ConfigSoundStartDelay,
		ConfigBluetoothEnable, ConfigWifiEnable, ConfigWifiMode, ConfigWifiSSID,
		ConfigWifiPassword, ConfigWifiPort, ConfigWifiState, ConfigWifiApIpAddress,
#if defined(MQTT_SUPPORT)
		MqttConfigEnable, MqttConfigBroker, MqttConfigPort, MqttConfigUsername,
		MqttConfigPassword, MqttConfigDeviceId, MqttConfigHADiscovery, MqttConfigKeepAlive,
		MqttConfigState, MqttConfigDiscoveryPrefix,
#endif
		ConfigTimeZoneOffset, ConfigMmsi, ConfigCallSign, ConfigHomePort,
		ConfigLedColor, ConfigLedBrightness, ConfigLedAutoSwitch, ConfigLedEnable,
		ControlPanelTones,
		ConfigReloadFromSd, ConfigExportToSd, ConfigSdCardSpeed, ConfigSdCardCsPin,
		ConfigLightSensorThreshold
	};
	count = sizeof(cmds) / sizeof(cmds[0]);
	return cmds;
}
