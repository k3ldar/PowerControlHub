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
#include "RelayController.h"

ConfigNetworkHandler::ConfigNetworkHandler(ConfigController* configController, WifiController* wifiController, RelayController* relayController)
	: _configController(configController), _wifiController(wifiController), _relayController(relayController)
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
	if (SystemFunctions::commandMatches(command, ConfigSaveSettings))
	{
		result = _configController->save();
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
	else if (SystemFunctions::commandMatches(command, ConfigXpdzTonePin))
	{
		// C6 - Set XpdzTone pin
		// Format: C6:v=<pin> (use 255/PinDisabled to disable)
		if (paramCount >= 1)
		{
			uint8_t pin;
			if (!getParamValueU8t(params, paramCount, "v", pin))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setXpdzTonePin(pin);
			}
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
	else if (SystemFunctions::commandMatches(command, RelayRename))
	{
		// R6: <idx>=<shortName|longName>
		if (paramCount >= 1)
		{
			uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
			if (idx >= ConfigRelayCount)
			{
				result = ConfigResult::InvalidRelay;
			}
			else
			{
				int pipeIdx = SystemFunctions::indexOf(params[0].value, '|', 0);
				char shortName[ConfigShortRelayNameLength] = "";
				char longName[ConfigLongRelayNameLength] = "";
				if (pipeIdx >= 0)
				{
					SystemFunctions::substr(shortName, sizeof(shortName), params[0].value, 0, pipeIdx);
					SystemFunctions::substr(longName, sizeof(longName), params[0].value, pipeIdx + 1);
				}
				else
				{
					strncpy(shortName, params[0].value, sizeof(shortName) - 1);
				}
				result = (_relayController->renameRelay(idx, shortName, longName) == RelayResult::Success)
					? ConfigResult::Success : ConfigResult::Failed;
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, RelaySetButtonColor))
	{
		// R7: <relay>=<color>
		if (paramCount >= 1)
		{
			uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
			uint8_t color = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

			if (relayIndex >= ConfigRelayCount)
			{
				result = ConfigResult::InvalidRelay;
			}
			else
			{
				if (color < DefaultValue)
					color += 2;
				result = (_relayController->setButtonColor(relayIndex, color) == RelayResult::Success)
					? ConfigResult::Success : ConfigResult::Failed;
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
			uint8_t type;
			if (!getParamValueU8t(params, paramCount, "v", type))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setLocationType(type);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigHw479RgbPins))
	{
		// C8 - Set Hw479Rgb RGB LED pins
		// Format: C8:r=<pin>;g=<pin>;b=<pin> (use 255/PinDisabled to disable)
		if (paramCount >= 3)
		{
			uint8_t rPin, gPin, bPin;
			if (!getParamValueU8t(params, paramCount, "r", rPin) ||
				!getParamValueU8t(params, paramCount, "g", gPin) ||
				!getParamValueU8t(params, paramCount, "b", bPin))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setHw479RgbPins(rPin, gPin, bPin);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigSoundStartDelay))
	{
		if (paramCount >= 1)
		{
			uint16_t soundStartDelay;
			if (!getParamValueU16t(params, paramCount, "v", soundStartDelay))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setsoundDelayStart(soundStartDelay);
			}
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
			bool enable;
			
			if (!getParamValueBool(params, paramCount, "v", enable))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setBluetoothEnabled(enable);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigWifiEnable))
	{
		// Expect "C11:v=<0|1>"
		bool enable;

		if (!getParamValueBool(params, paramCount, "v", enable))
		{
			result = ConfigResult::InvalidParameter;
		}
		else
		{
			result = _configController->setWifiEnabled(enable);
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigWifiMode))
	{
		// Expect "C12:v=<0|1>"
		if (paramCount >= 1)
		{
			uint8_t modeValue;
			if (!getParamValueU8t(params, paramCount, "v", modeValue))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				WifiMode mode = static_cast<WifiMode>(modeValue);
				result = _configController->setWifiAccessMode(mode);
			}
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
			uint16_t port;
			if (!getParamValueU16t(params, paramCount, "v", port))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setWifiPort(port);
			}
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
			state = static_cast<uint8_t>(_wifiController->getServer()->getConnectionState());
		}

		int len = snprintf(responseBuffer, bufferSize, "v=%d", state);
		result = (len > 0 && len < static_cast<int>(bufferSize)) ? ConfigResult::Success : ConfigResult::InvalidParameter;
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
	else if (SystemFunctions::commandMatches(command, ConfigRtcPins))
	{
		// C18 - Set RtcConfig DS1302 pins
		// Format: C18:dat=<pin>;clk=<pin>;rst=<pin> (use 255/PinDisabled for any unfit pin)
		if (paramCount >= 3)
		{
			uint8_t dataPin, clockPin, resetPin;
			if (!getParamValueU8t(params, paramCount, "dat", dataPin) ||
				!getParamValueU8t(params, paramCount, "clk", clockPin) ||
				!getParamValueU8t(params, paramCount, "rst", resetPin))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setRtcPins(dataPin, clockPin, resetPin);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionGetConfig))
	{
		// N0 - returns all nextion settings as a JSON-style response in responseBuffer
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else
		{
			int len = snprintf(responseBuffer, bufferSize,
				"en=%u;hw=%u;rx=%u;tx=%u;baud=%lu;uart=%u",
				config->nextion.enabled ? 1u : 0u,
				config->nextion.isHardwareSerial ? 1u : 0u,
				config->nextion.rxPin,
				config->nextion.txPin,
				config->nextion.baudRate,
				config->nextion.uartNum);
			result = (len > 0 && len < static_cast<int>(bufferSize))
				? ConfigResult::Success : ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionEnabled))
	{
		if (paramCount >= 1)
		{
			bool enabled;
			if (!getParamValueBool(params, paramCount, "v", enabled))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionEnabled(enabled);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionHardwareSerial))
	{
		if (paramCount >= 1)
		{
			bool hwSerial;
			if (!getParamValueBool(params, paramCount, "v", hwSerial))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionHardwareSerial(hwSerial);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionRxPin))
	{
		if (paramCount >= 1)
		{
			uint8_t pin;
			if (!getParamValueU8t(params, paramCount, "v", pin))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionRxPin(pin);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionTxPin))
	{
		if (paramCount >= 1)
		{
			uint8_t pin;
			if (!getParamValueU8t(params, paramCount, "v", pin))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionTxPin(pin);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionBaudRate))
	{
		if (paramCount >= 1)
		{
			uint32_t baud;
			if (!getParamValueU32t(params, paramCount, "v", baud))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionBaudRate(baud);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, NextionUartNum))
	{
		if (paramCount >= 1)
		{
			uint8_t uartNum;
			if (!getParamValueU8t(params, paramCount, "v", uartNum))
				result = ConfigResult::InvalidParameter;
			else
				result = _configController->setNextionUartNum(uartNum);
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigTimeZoneOffset))
	{
		// C20 - Set timezone offset
		if (paramCount >= 1)
		{
			int8_t offset;
			if (!getParamValue8t(params, paramCount, "v", offset))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setTimezoneOffset(offset);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigMmsi))
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
	else if (SystemFunctions::commandMatches(command, ConfigCallSign))
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
	else if (SystemFunctions::commandMatches(command, ConfigHomePort))
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
	else if (SystemFunctions::commandMatches(command, ConfigLedColor))
	{
		// C24 - Set LED RGB color
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
		if (paramCount >= 1)
		{
			bool enabled;
			if (!getParamValueBool(params, paramCount, "v", enabled))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setLedAutoSwitch(enabled);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigLedEnable))
	{
		// C27 - Enable/disable individual LEDs
		if (paramCount >= 3)
		{
			bool gps, warning, system;

			if (!getParamValueBool(params, paramCount, "g", gps) ||
				!getParamValueBool(params, paramCount, "w", warning) ||
				!getParamValueBool(params, paramCount, "s", system))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setLedEnableStates(gps, warning, system);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ControlPanelTones))
	{
		// C28 - Configure control panel tones
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
	else if (SystemFunctions::commandMatches(command, ConfigSdCardSpeed))
	{
		// C31 - Set SD card initialize speed
		// Format: C31:v=4 (or 8, 12, 16, 20, 24)
		if (paramCount >= 1)
		{
			uint8_t speedMhz;
			if (!getParamValueU8t(params, paramCount, "v", speedMhz))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setSdCardInitializeSpeed(speedMhz);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ConfigSdCardCsPin))
	{
		// C32 - Set SD card CS pin
		// Format: C32:v=10
		if (paramCount >= 1)
		{
			uint8_t csPin;
			if (!getParamValueU8t(params, paramCount, "v", csPin))
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				result = _configController->setSdCardCsPin(csPin);
			}
		}
		else
		{
			result = ConfigResult::InvalidParameter;
		}
	}
	else if (SystemFunctions::commandMatches(command, ExternalSensorGetAll))
	{
		// E0 — return all remote sensor config entries as JSON array
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else
		{
			int written = snprintf(responseBuffer, bufferSize,
				"\"count\":%u,\"sensors\":[", config->remoteSensors.count);

			for (uint8_t i = 0; i < config->remoteSensors.count && i < ConfigMaxSensors; i++)
			{
				const RemoteSensorConfig& e = config->remoteSensors.sensors[i];
				int n = snprintf(responseBuffer + written, bufferSize - written,
					"%s{\"i\":%u,\"id\":%u,\"n\":\"%s\",\"mn\":\"%s\",\"ms\":\"%s\","
					"\"mt\":\"%s\",\"md\":\"%s\",\"mu\":\"%s\",\"bin\":%u}",
					i > 0 ? "," : "",
					i,
					static_cast<uint8_t>(e.sensorId),
					e.name,
					e.mqttName,
					e.mqttSlug,
					e.mqttTypeSlug,
					e.mqttDeviceClass,
					e.mqttUnit,
					e.mqttIsBinary ? 1u : 0u);
				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;
				written += n;
			}

			snprintf(responseBuffer + written, bufferSize - written, "]");
			result = ConfigResult::Success;
		}
	}
	else if (SystemFunctions::commandMatches(command, ExternalSensorSetCore))
	{
		// E1:i=<idx>;id=<sensorId>;n=<name>;mn=<mqttName>;ms=<mqttSlug>
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else
		{
			uint8_t idx, sensorId;
			if (!getParamValueU8t(params, paramCount, "i", idx) ||
				!getParamValueU8t(params, paramCount, "id", sensorId) ||
				idx >= ConfigMaxSensors)
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];
				entry.sensorId = static_cast<SensorIdList>(sensorId);

				const char* n = getParamValue(params, paramCount, "n");
				if (n && n[0] != '\0')
				{
					strncpy(entry.name, n, sizeof(entry.name) - 1);
					entry.name[sizeof(entry.name) - 1] = '\0';
				}

				const char* mn = getParamValue(params, paramCount, "mn");
				if (mn)
				{
					strncpy(entry.mqttName, mn, sizeof(entry.mqttName) - 1);
					entry.mqttName[sizeof(entry.mqttName) - 1] = '\0';
				}

				const char* ms = getParamValue(params, paramCount, "ms");
				if (ms)
				{
					strncpy(entry.mqttSlug, ms, sizeof(entry.mqttSlug) - 1);
					entry.mqttSlug[sizeof(entry.mqttSlug) - 1] = '\0';
				}

				if (idx >= config->remoteSensors.count)
					config->remoteSensors.count = idx + 1;

				result = ConfigResult::Success;
			}
		}
	}
	else if (SystemFunctions::commandMatches(command, ExternalSensorSetMqtt))
	{
		// E2:i=<idx>;mt=<mqttTypeSlug>;md=<mqttDeviceClass>;mu=<mqttUnit>;bin=<0|1>
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else
		{
			uint8_t idx;
			if (!getParamValueU8t(params, paramCount, "i", idx) ||
				idx >= ConfigMaxSensors || idx >= config->remoteSensors.count)
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];

				const char* mt = getParamValue(params, paramCount, "mt");
				if (mt)
				{
					strncpy(entry.mqttTypeSlug, mt, sizeof(entry.mqttTypeSlug) - 1);
					entry.mqttTypeSlug[sizeof(entry.mqttTypeSlug) - 1] = '\0';
				}

				const char* md = getParamValue(params, paramCount, "md");
				if (md)
				{
					strncpy(entry.mqttDeviceClass, md, sizeof(entry.mqttDeviceClass) - 1);
					entry.mqttDeviceClass[sizeof(entry.mqttDeviceClass) - 1] = '\0';
				}

				const char* mu = getParamValue(params, paramCount, "mu");
				if (mu)
				{
					strncpy(entry.mqttUnit, mu, sizeof(entry.mqttUnit) - 1);
					entry.mqttUnit[sizeof(entry.mqttUnit) - 1] = '\0';
				}

				bool isBinary;
				if (getParamValueBool(params, paramCount, "bin", isBinary))
					entry.mqttIsBinary = isBinary;

				result = ConfigResult::Success;
			}
		}
	}
	else if (SystemFunctions::commandMatches(command, ExternalSensorRemove))
	{
		// E3:<idx>
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else if (paramCount < 1)
		{
			result = ConfigResult::InvalidParameter;
		}
		else
		{
			uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));
			if (idx >= ConfigMaxSensors || idx >= config->remoteSensors.count)
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				for (uint8_t j = idx; j + 1 < config->remoteSensors.count; j++)
					config->remoteSensors.sensors[j] = config->remoteSensors.sensors[j + 1];

				if (config->remoteSensors.count > 0)
					config->remoteSensors.count--;

				memset(&config->remoteSensors.sensors[config->remoteSensors.count], 0,
					sizeof(RemoteSensorConfig));

				result = ConfigResult::Success;
			}
		}
	}
	else if (SystemFunctions::commandMatches(command, ExternalSensorRename))
	{
		// E4:<idx>=<name>
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr)
		{
			result = ConfigResult::InvalidConfig;
		}
		else if (paramCount < 1)
		{
			result = ConfigResult::InvalidParameter;
		}
		else
		{
			uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
			if (idx >= ConfigMaxSensors || idx >= config->remoteSensors.count || params[0].value[0] == '\0')
			{
				result = ConfigResult::InvalidParameter;
			}
			else
			{
				RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];
				strncpy(entry.name, params[0].value, sizeof(entry.name) - 1);
				entry.name[sizeof(entry.name) - 1] = '\0';
				result = ConfigResult::Success;
			}
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
	client->print(config->location.name);
	client->print("\",");

	client->print("\"spiPins\":{");
	client->print("\"sck\":");
	client->print(config->spiPins.sckPin);
	client->print(",");
	client->print("\"mosi\":");
	client->print(config->spiPins.mosiPin);
	client->print(",");
	client->print("\"miso\":");
	client->print(config->spiPins.misoPin);
	client->print("},");

	// Vessel type
	client->print("\"vesselType\":");
	client->print(static_cast<uint8_t>(config->location.locationType));
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

	// C20 Timezone offset
	client->print("\"timezoneOffset\":");
	client->print(static_cast<int>(config->system.timezoneOffset));
	client->print(",");

	// C21 MMSI
	client->print("\"mmsi\":\"");
	client->print(config->location.mmsi);
	client->print("\",");

	// C22 Call sign
	client->print("\"callSign\":\"");
	client->print(config->location.callSign);
	client->print("\",");

	// C23 Home port
	client->print("\"homePort\":\"");
	client->print(config->location.homePort);
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
	client->print(",");

	// C32 SD Card CS pin
	client->print("\"sdCardCsPin\":");
	client->print(config->sdCard.csPin);

	client->print("}");
}

void ConfigNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	formatStatusJson(client);
}
