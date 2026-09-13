/*
 * PowerControlHub
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
#include "SensorNetworkHandler.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"
#include "ConfigManager.h"
#include "SensorConfig.h"
#include "PinGuard.h"


SensorNetworkHandler::SensorNetworkHandler(SensorController* sensorController)
	: _sensorController(sensorController)
{
}

CommandResult SensorNetworkHandler::handleRequest(const char* method,
	const char* command, StringKeyValue* params, uint8_t paramCount,
	char* responseBuffer, size_t bufferSize)
{
	(void)method;

	Config* config = ConfigManager::getConfigPtr();

	// ---- S0: Get All Sensor Config (or no command returns full status) ----
	// Supports ?meta=1 to append sensor type descriptors for self-describing UI
	if (SystemFunctions::commandMatches(command, SensorConfigGetAll) || command[0] == '\0')
	{
		bool includeMeta = false;
		getParamValueBool(params, paramCount, "meta", includeMeta);

		int written = snprintf(responseBuffer, bufferSize,
			"\"success\":true,\"count\":%u,\"sensors\":[", config ? config->sensors.count : 0);

		if (config)
		{
			for (uint8_t i = 0; i < config->sensors.count && i < ConfigMaxSensors; i++)
			{
				const SensorEntry& e = config->sensors.sensors[i];
				int n = snprintf(responseBuffer + written, bufferSize - written,
					"%s{\"i\":%u,\"t\":%u,\"n\":\"%s\","
					"\"p0\":%u,\"p1\":%u,"
					"\"u0\":%d,\"u1\":%d,"
					"\"o0\":%d,\"o1\":%d,"
					"\"en\":%u}",
					i > 0 ? "," : "",
					i,
					static_cast<uint8_t>(e.sensorType),
					e.name,
					e.pins[0],
					e.pins[1],
					e.options1[0],
					e.options1[1],
					e.options2[0],
					e.options2[1],
					e.enabled ? 1u : 0u);
				
				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;

				written += n;
			}
		}

		snprintf(responseBuffer + written, bufferSize - written, "]");

		// Append sensor type descriptors when meta=1
		if (includeMeta)
		{
			written = static_cast<int>(strnlen(responseBuffer, bufferSize));
			int n = snprintf(responseBuffer + written, bufferSize - written,
				",\"meta\":{\"count\":%u,\"descriptors\":[",
				static_cast<unsigned>(SensorIdList::Count));

			if (n < 0 || written + n >= static_cast<int>(bufferSize))
			{
				responseBuffer[bufferSize - 1] = '\0';
				return CommandResult::ok();
			}

			written += n;

			for (unsigned di = 0; di < static_cast<unsigned>(SensorIdList::Count); di++)
			{
				const auto& d = SensorDescriptors[di];
				n = snprintf(responseBuffer + written, bufferSize - written,
					"%s{\"id\":%u,\"name\":\"%s\","
					"\"pins\":[",
					di > 0 ? "," : "", di, d.name);

				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;

				written += n;
				bool firstPin = true;

				for (unsigned p = 0; p < ConfigMaxSensorPins; p++)
				{
					if (strcmp(d.pins[p].type, "none") == 0)
						continue;

					n = snprintf(responseBuffer + written, bufferSize - written,
						"%s{\"label\":\"%s\",\"type\":\"%s\",\"min\":%d,\"max\":%d,\"default\":%d}",
						firstPin ? "" : ",",
						d.pins[p].label, d.pins[p].type,
						d.pins[p].minVal, d.pins[p].maxVal, d.pins[p].defaultValue);
					
					if (n < 0 || written + n >= static_cast<int>(bufferSize))
						break;

					written += n;
					firstPin = false;
				}

				n = snprintf(responseBuffer + written, bufferSize - written,
					"],\"options1\":[");
				
				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;

				written += n;

				bool firstOpt1 = true;

				for (unsigned o = 0; o < 2; o++)
				{
					if (strcmp(d.options1[o].type, "none") == 0)
						continue;

					n = snprintf(responseBuffer + written, bufferSize - written,
						"%s{\"label\":\"%s\",\"type\":\"%s\",\"min\":%d,\"max\":%d,\"default\":%d}",
						firstOpt1 ? "" : ",",
						d.options1[o].label, d.options1[o].type,
						d.options1[o].minVal, d.options1[o].maxVal, d.options1[o].defaultValue);
					
					if (n < 0 || written + n >= static_cast<int>(bufferSize))
						break;

					written += n;
					firstOpt1 = false;
				}

				n = snprintf(responseBuffer + written, bufferSize - written,
					"],\"options2\":[");
				
				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;

				written += n;
				bool firstOpt2 = true;

				for (unsigned o = 0; o < 2; o++)
				{
					if (strcmp(d.options2[o].type, "none") == 0)
						continue;

					n = snprintf(responseBuffer + written, bufferSize - written,
						"%s{\"label\":\"%s\",\"type\":\"%s\",\"min\":%d,\"max\":%d,\"default\":%d}",
						firstOpt2 ? "" : ",",
						d.options2[o].label, d.options2[o].type,
						d.options2[o].minVal, d.options2[o].maxVal, d.options2[o].defaultValue);
					
					if (n < 0 || written + n >= static_cast<int>(bufferSize))
						break;

					written += n;
					firstOpt2 = false;
				}

				n = snprintf(responseBuffer + written, bufferSize - written, "]}");
				
				if (n < 0 || written + n >= static_cast<int>(bufferSize))
					break;

				written += n;
			}

			snprintf(responseBuffer + written, bufferSize - written, "]}");
		}

		return CommandResult::ok();
	}

	// ---- S1: Add / Update Sensor ----
	if (SystemFunctions::commandMatches(command, SensorConfigAddUpdate))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx, type;
		int8_t opt0, opt1;

		if (!getParamValueU8t(params, paramCount, "i", idx) ||
			!getParamValueU8t(params, paramCount, "t", type) ||
			!getParamValue8t(params, paramCount, "o0", opt0) ||
			!getParamValue8t(params, paramCount, "o1", opt1) ||
			idx >= ConfigMaxSensors)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		SensorEntry& entry = config->sensors.sensors[idx];
		entry.enabled = true;
		entry.sensorType = static_cast<SensorIdList>(type);
		entry.options1[0] = opt0;
		entry.options1[1] = opt1;

		if (idx >= config->sensors.count)
			config->sensors.count = idx + 1;

		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S2: Remove Sensor ----
	if (SystemFunctions::commandMatches(command, SensorConfigRemove))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx;

		if (paramCount < 1 || !getParamValueU8t(params, paramCount, "v", idx))
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		if (idx >= ConfigMaxSensors)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		if (idx < config->sensors.count)
		{
			for (uint8_t j = idx; j + 1 < config->sensors.count; j++)
				config->sensors.sensors[j] = config->sensors.sensors[j + 1];

			if (config->sensors.count > 0)
				config->sensors.count--;

			memset(&config->sensors.sensors[config->sensors.count], 0, sizeof(SensorEntry));
		}

		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S3: Rename Sensor ----
	if (SystemFunctions::commandMatches(command, SensorConfigRename))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		if (paramCount < 1)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));

		if (idx >= ConfigMaxSensors || idx >= config->sensors.count || params[0].value[0] == '\0')
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		SensorEntry& entry = config->sensors.sensors[idx];
		strncpy(entry.name, params[0].value, sizeof(entry.name) - 1);
		entry.name[sizeof(entry.name) - 1] = '\0';
		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S4: Set Sensor Pin ----
	if (SystemFunctions::commandMatches(command, SensorConfigSetPin))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx, slot, pin;

		if (!getParamValueU8t(params, paramCount, "i", idx) ||
			!getParamValueU8t(params, paramCount, "s", slot) ||
			!getParamValueU8t(params, paramCount, "v", pin) ||
			idx >= ConfigMaxSensors || idx >= config->sensors.count ||
			slot >= ConfigMaxSensorPins)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		// Validate pin through PinGuard — only GPIO slots represent actual hardware pins
		{
			const uint8_t sensorType = static_cast<uint8_t>(config->sensors.sensors[idx].sensorType);
			bool isGpioSlot = false;
			PinUse intendedUse = PinUse::Sensor;

			if (sensorType < static_cast<uint8_t>(SensorIdList::Count))
			{
				const SensorFieldDescriptor& field = SensorDescriptors[sensorType].pins[slot];
				isGpioSlot = (strcmp(field.type, "gpio") == 0);
				intendedUse = field.pinUse;
			}

			// Always validate GPIO slots through PinGuard (covers hard-blocks, advisory, and InUse).
			// Non-GPIO slots store payload bytes (relay indices, action types) — skip PinGuard.
			if (isGpioSlot && PinGuard::isBlocked(PinGuard::validate(pin, intendedUse)))
			{
				formatJsonResponse(responseBuffer, bufferSize, false, "Pin blocked by PinGuard");
				return CommandResult::error(InvalidCommandParameters);
			}
		}

		config->sensors.sensors[idx].pins[slot] = pin;
		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S5: Set Sensor Enabled ----
	if (SystemFunctions::commandMatches(command, SensorConfigSetEnabled))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		if (paramCount < 1)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
		bool enabled = SystemFunctions::parseBooleanValue(params[0].value);

		if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		config->sensors.sensors[idx].enabled = enabled;
		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S6: Set Sensor Option ----
	if (SystemFunctions::commandMatches(command, SensorConfigSetOptions))
	{
		if (!config)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
			return CommandResult::error(InvalidCommandParameters);
		}

		uint8_t idx, slot, option;

		if (!getParamValueU8t(params, paramCount, "i", idx) ||
			!getParamValueU8t(params, paramCount, "s", slot) ||
			!getParamValueU8t(params, paramCount, "o", option) ||
			idx >= ConfigMaxSensors || idx >= config->sensors.count ||
			option > 1)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		constexpr uint8_t options1Size = sizeof(SensorEntry::options1) / sizeof(SensorEntry::options1[0]);

		if (slot >= options1Size)
		{
			return CommandResult::error(InvalidCommandParameters);
		}

		if (option == 0)
		{
			int8_t val8;

			if (!getParamValue8t(params, paramCount, "v", val8))
			{
				return CommandResult::error(InvalidCommandParameters);
			}

			config->sensors.sensors[idx].options1[slot] = val8;
		}
		else
		{
			int16_t val16;

			if (!getParamValue16t(params, paramCount, "v", val16))
			{
				return CommandResult::error(InvalidCommandParameters);
			}

			config->sensors.sensors[idx].options2[slot] = val16;
		}

		formatJsonResponse(responseBuffer, bufferSize, true);
		return CommandResult::ok();
	}

	// ---- S7-S24: Sensor Telemetry Queries (return full status) ----
	if (SystemFunctions::commandMatches(command, SensorTemperature) ||
		SystemFunctions::commandMatches(command, SensorHumidity) ||
		SystemFunctions::commandMatches(command, SensorBearing) ||
		SystemFunctions::commandMatches(command, SensorDirection) ||
		SystemFunctions::commandMatches(command, SensorSpeed) ||
		SystemFunctions::commandMatches(command, SensorCompassTemp) ||
		SystemFunctions::commandMatches(command, SensorWaterLevel) ||
		SystemFunctions::commandMatches(command, SensorWaterPumpActive) ||
		SystemFunctions::commandMatches(command, SensorHornActive) ||
		SystemFunctions::commandMatches(command, SensorLightSensor) ||
		SystemFunctions::commandMatches(command, SensorGpsLatLong) ||
		SystemFunctions::commandMatches(command, SensorGpsAltitude) ||
		SystemFunctions::commandMatches(command, SensorGpsSpeed) ||
		SystemFunctions::commandMatches(command, SensorGpsSatellites) ||
		SystemFunctions::commandMatches(command, SensorGpsDistance) ||
		SystemFunctions::commandMatches(command, SensorBinaryPresence) ||
		SystemFunctions::commandMatches(command, SensorVoltage) ||
		SystemFunctions::commandMatches(command, SensorReed))
	{
		if (!_sensorController)
		{
			formatJsonResponse(responseBuffer, bufferSize, false, "Sensor controller not initialized");
			return CommandResult::error(InvalidCommandParameters);
		}

		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}

	// ---- Unknown command ----
	formatJsonResponse(responseBuffer, bufferSize, false, "Unknown sensor command");
	return CommandResult::error(InvalidCommandParameters);
}

void SensorNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	if (!buffer || size == 0)
	{
		return;
	}

	if (!_sensorController)
	{
		int written = snprintf(buffer, size, "\"sensors\":{}");
		if (written < 0 || written >= static_cast<int>(size))
		{
			buffer[size - 1] = '\0';
		}
		return;
	}

	int written = snprintf(buffer, size, "\"sensors\":{");
	if (written < 0 || written >= static_cast<int>(size))
	{
		buffer[size - 1] = '\0';
		return; // Buffer too small for even the opening
	}

	bool firstSensor = true;
	for (uint8_t i = 0; i < _sensorController->sensorCount(); i++)
	{
		BaseSensor* sensor = _sensorController->sensorGet(i);

		if (sensor == nullptr)
			continue;

		char sensorBuffer[MaximumJsonResponseBufferSize];
		sensorBuffer[0] = '\0';

		sensor->formatStatusJson(sensorBuffer, sizeof(sensorBuffer));

		if (sensorBuffer[0] == '\0')
			continue;

		// Add comma separator if not the first element
		if (!firstSensor)
		{
			int n = snprintf(buffer + written, size - written, ",");
			if (n < 0 || written + n >= static_cast<int>(size))
			{
				buffer[size - 1] = '\0';
				return; // Out of space
			}
			written += n;
		}

		// Write sensor entry (removed duplicate and fixed format)
		char safeName[64];
		SystemFunctions::sanitizeJsonString(sensor->getSensorName(), safeName, sizeof(safeName));
		int n = snprintf(buffer + written, size - written,
			"\"%s\":{\"uid\":%d,\"idType\":%d,\"type\":%d,%s}",
			safeName,
			sensor->getUid(),
			static_cast<uint8_t>(sensor->getSensorIdType()),
			static_cast<uint8_t>(sensor->getSensorType()),
			sensorBuffer);

		if (n < 0 || written + n >= static_cast<int>(size))
		{
			buffer[size - 1] = '\0';
			return; // Out of space
		}
		written += n;
		firstSensor = false;
	}

	// Close JSON object
	int n = snprintf(buffer + written, size - written, "}");
	if (n < 0 || written + n >= static_cast<int>(size))
	{
		buffer[size - 1] = '\0';
	}
}

void SensorNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
