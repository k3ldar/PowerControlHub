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
#include "ExternalSensorNetworkHandler.h"

#include "ConfigManager.h"
#include "ConfigController.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"

CommandResult ExternalSensorNetworkHandler::handleRequest(const char* method,
    const char* command,
    StringKeyValue* params,
    uint8_t paramCount,
    char* responseBuffer,
    size_t bufferSize)
{
    (void)method;

    Config* config = ConfigManager::getConfigPtr();
    if (!config)
    {
        formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
        return CommandResult::error(InvalidConfiguration);
    }

    ConfigResult result = ConfigResult::InvalidCommand;

    if (SystemFunctions::commandMatches(command, ExternalSensorGetAll) || command[0] == '\0')
    {
        // E0 — return all remote sensor config entries
        int written = snprintf(responseBuffer, bufferSize,
            "\"success\":true,\"count\":%u,\"sensors\":[", config->remoteSensors.count);

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
        return CommandResult::ok();
    }
    else if (SystemFunctions::commandMatches(command, ExternalSensorSetCore))
    {
        // E1:i=<idx>;id=<sensorId>;n=<name>;mn=<mqttName>;ms=<mqttSlug>
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
    else if (SystemFunctions::commandMatches(command, ExternalSensorSetMqtt))
    {
        // E2:i=<idx>;mt=<mqttTypeSlug>;md=<mqttDeviceClass>;mu=<mqttUnit>;bin=<0|1>
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
    else if (SystemFunctions::commandMatches(command, ExternalSensorRemove))
    {
        // E3:<idx>
        if (paramCount < 1)
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
        if (paramCount < 1)
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

    if (result == ConfigResult::Success)
        return CommandResult::ok();

    return CommandResult::error(static_cast<uint8_t>(result));
}

void ExternalSensorNetworkHandler::formatStatusJson(IWifiClient* client)
{
    Config* config = ConfigManager::getConfigPtr();
    if (!config)
        return;

    client->print("\"externalSensors\":{");
    client->print("\"count\":");
    client->print(config->remoteSensors.count);
    client->print(",\"sensors\":[");

    for (uint8_t i = 0; i < config->remoteSensors.count && i < ConfigMaxSensors; i++)
    {
        const RemoteSensorConfig& e = config->remoteSensors.sensors[i];

        if (i > 0)
            client->print(",");

        client->print("{\"i\":");
        client->print(i);
        client->print(",\"id\":");
        client->print(static_cast<uint8_t>(e.sensorId));
        client->print(",\"n\":\"");
        client->print(e.name);
        client->print("\",\"mn\":\"");
        client->print(e.mqttName);
        client->print("\",\"ms\":\"");
        client->print(e.mqttSlug);
        client->print("\",\"mt\":\"");
        client->print(e.mqttTypeSlug);
        client->print("\",\"md\":\"");
        client->print(e.mqttDeviceClass);
        client->print("\",\"mu\":\"");
        client->print(e.mqttUnit);
        client->print("\",\"bin\":");
        client->print(e.mqttIsBinary ? "true" : "false");
        client->print("}");
    }

    client->print("]}");
}

void ExternalSensorNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
    formatStatusJson(client);
}
