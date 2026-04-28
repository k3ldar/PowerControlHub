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

#include "BaseConfigCommandHandler.h"
#include "SystemDefinitions.h"
#include "Config.h"
#include "ConfigManager.h"

/**
 * @brief Command handler for remote/external sensor configuration (E-series commands).
 *
 * Manages the RemoteSensorsConfig array that drives dynamic RemoteSensor creation at
 * boot. Because each RemoteSensorConfig has more fields than the 5-parameter limit,
 * add/update is split across two commands.
 *
 * *** Important — reboot required ***
 * RemoteSensor instances are constructed once during SmartFuseBoxApp::setup().
 * Changes here only affect persisted config; they take effect at next boot.
 * Every mutating ACK carries a "reboot=1" field to make this explicit.
 *
 * Command reference:
 *   E0          — Get all external sensor config entries (broadcasts one E0 per entry)
 *   E1:i=<idx>;id=<sensorId>;n=<name>;mn=<mqttName>;ms=<mqttSlug>
 *               — Set core fields for sensor at <idx> (0-based, up to ConfigMaxSensors)
 *   E2:i=<idx>;mt=<mqttTypeSlug>;md=<mqttDeviceClass>;mu=<mqttUnit>;bin=<0|1>
 *               — Set MQTT detail fields for sensor at <idx>
 *   E3:<idx>    — Remove sensor at <idx> (clears entry, shifts count down)
 *   E4:<idx>=<name>
 *               — Rename sensor at <idx>
 */
class ExternalSensorConfigCommandHandler : public virtual BaseCommandHandler, public BaseConfigCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;

    void broadcastSensorConfig(SerialCommandManager* sender, uint8_t index, const RemoteSensorConfig& entry)
    {
        char buf[96];
        snprintf(buf, sizeof(buf),
            "i=%u;id=%u;n=%s;mn=%s;ms=%s",
            index,
            static_cast<uint8_t>(entry.sensorId),
            entry.name,
            entry.mqttName,
            entry.mqttSlug);
        if (_commandMgrComputer)
            _commandMgrComputer->sendCommand(ExternalSensorGetAll, buf);

        snprintf(buf, sizeof(buf),
            "i=%u;mt=%s;md=%s;mu=%s;bin=%u",
            index,
            entry.mqttTypeSlug,
            entry.mqttDeviceClass,
            entry.mqttUnit,
            entry.mqttIsBinary ? 1u : 0u);
        if (_commandMgrComputer)
            _commandMgrComputer->sendCommand(ExternalSensorGetAll, buf);
    }

public:
    explicit ExternalSensorConfigCommandHandler(SerialCommandManager* commandMgrComputer)
        : _commandMgrComputer(commandMgrComputer)
    {
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        static const char* cmds[] = {
            ExternalSensorGetAll,
            ExternalSensorSetCore,
            ExternalSensorSetMqtt,
            ExternalSensorRemove,
            ExternalSensorRename
        };
        count = sizeof(cmds) / sizeof(cmds[0]);
        return cmds;
    }

    bool handleCommand(SerialCommandManager* sender, const char* command,
                       const StringKeyValue params[], uint8_t paramCount) override
    {
        Config* config = ConfigManager::getConfigPtr();

        if (config == nullptr)
        {
            sendAckErr(sender, command, F("Config not available"));
            return true;
        }

        if (SystemFunctions::commandMatches(command, ExternalSensorGetAll))
        {
            for (uint8_t i = 0; i < config->remoteSensors.count && i < ConfigMaxSensors; i++)
            {
                broadcastSensorConfig(sender, i, config->remoteSensors.sensors[i]);
            }

            sendAckOk(sender, command);
            return true;
        }

        if (SystemFunctions::commandMatches(command, ExternalSensorSetCore))
        {
            // E1:i=<idx>;id=<sensorId>;n=<name>;mn=<mqttName>;ms=<mqttSlug>
            uint8_t idx, sensorId;
            if (!getParamValueU8t(params, paramCount, "i", idx) ||
                !getParamValueU8t(params, paramCount, "id", sensorId))
            {
                sendAckErr(sender, command, F("Invalid or missing parameters"));
                return true;
            }

            if (idx >= ConfigMaxSensors)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];
            entry.sensorId = static_cast<SensorIdList>(sensorId);

            const char* name = getParamValue(params, paramCount, "n");
            if (name && name[0] != '\0')
            {
                strncpy(entry.name, name, sizeof(entry.name) - 1);
                entry.name[sizeof(entry.name) - 1] = '\0';
            }

            const char* mqttName = getParamValue(params, paramCount, "mn");
            if (mqttName)
            {
                strncpy(entry.mqttName, mqttName, sizeof(entry.mqttName) - 1);
                entry.mqttName[sizeof(entry.mqttName) - 1] = '\0';
            }

            const char* mqttSlug = getParamValue(params, paramCount, "ms");
            if (mqttSlug)
            {
                strncpy(entry.mqttSlug, mqttSlug, sizeof(entry.mqttSlug) - 1);
                entry.mqttSlug[sizeof(entry.mqttSlug) - 1] = '\0';
            }

            if (idx >= config->remoteSensors.count)
                config->remoteSensors.count = idx + 1;

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, ExternalSensorSetMqtt))
        {
            // E2:i=<idx>;mt=<mqttTypeSlug>;md=<mqttDeviceClass>;mu=<mqttUnit>;bin=<0|1>
            uint8_t idx;
            if (!getParamValueU8t(params, paramCount, "i", idx))
            {
                sendAckErr(sender, command, F("Invalid or missing parameters"));
                return true;
            }

            if (idx >= ConfigMaxSensors || idx >= config->remoteSensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

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

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, ExternalSensorRemove))
        {
            // E3:<idx>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing sensor index"));
                return true;
            }

            uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));

            if (idx >= ConfigMaxSensors || idx >= config->remoteSensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];
            entry.sensorId = static_cast<SensorIdList>(0);
            memset(entry.name, 0, sizeof(entry.name));
            memset(entry.mqttName, 0, sizeof(entry.mqttName));
            memset(entry.mqttSlug, 0, sizeof(entry.mqttSlug));
            memset(entry.mqttTypeSlug, 0, sizeof(entry.mqttTypeSlug));
            memset(entry.mqttDeviceClass, 0, sizeof(entry.mqttDeviceClass));
            memset(entry.mqttUnit, 0, sizeof(entry.mqttUnit));
            entry.mqttIsBinary = false;

            // Compact the array: shift entries down over the removed slot
            for (uint8_t j = idx; j + 1 < config->remoteSensors.count; j++)
                config->remoteSensors.sensors[j] = config->remoteSensors.sensors[j + 1];

            if (config->remoteSensors.count > 0)
                config->remoteSensors.count--;

            // Zero the vacated last slot
            RemoteSensorConfig& last = config->remoteSensors.sensors[config->remoteSensors.count];
            memset(&last, 0, sizeof(last));

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, ExternalSensorRename))
        {
            // E4:<idx>=<name>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing parameters"));
                return true;
            }

            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));

            if (idx >= ConfigMaxSensors || idx >= config->remoteSensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            if (params[0].value[0] == '\0')
            {
                sendAckErr(sender, command, F("Missing name"));
                return true;
            }

            RemoteSensorConfig& entry = config->remoteSensors.sensors[idx];
            strncpy(entry.name, params[0].value, sizeof(entry.name) - 1);
            entry.name[sizeof(entry.name) - 1] = '\0';

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        return false;
    }
};
