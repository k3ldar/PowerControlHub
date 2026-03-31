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

/**
 * @brief Command handler for persisted sensor configuration (N-series commands).
 *
 * Owns all mutations to SensorsConfig inside Config — adding, removing, renaming,
 * pin assignment, enable toggling and option tuning.  Changes are written to the
 * in-memory Config struct and must be persisted by the caller (C0 / ConfigSaveSettings)
 * before they survive a power cycle.
 *
 * *** Important — reboot required ***
 * SensorFactory constructs live sensor instances once at boot from SensorsConfig.
 * Runtime mutations here only alter persisted config; they take effect at next boot.
 * Every mutating ACK carries a "reboot=1" field to make this explicit to the client.
 *
 * Command reference:
 *   S0          — Get all sensor config entries
 *   S1:i=<idx>;t=<type>;n=<name>;o0=<opt0>;o1=<opt1>
 *               — Add or update sensor at <idx> (0-based, up to ConfigMaxSensors)
 *   S2:<idx>    — Remove sensor at <idx> (sets enabled=false, clears entry)
 *   S3:<idx>=<name>
 *               — Rename sensor at <idx>
 *   S4:<idx>;s=<slot>;v=<pin>
 *               — Set pin at slot (0..ConfigMaxSensorPins-1) for sensor <idx>
 *   S5:<idx>=<0|1>
 *               — Enable (1) or disable (0) sensor at <idx>
 *   S6:<idx>;s=<slot>;v=<value>
 *               — Set options1[slot] for sensor <idx>
 */
class SensorConfigCommandHandler : public BaseConfigCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;

    void broadcastSensorConfig(SerialCommandManager* sender, uint8_t index, const SensorEntry& entry)
    {
        char buf[96];
        snprintf(buf, sizeof(buf),
            "i=%u;t=%u;n=%s;p0=%u;p1=%u;o0=%d;o1=%d;en=%u",
            index,
            static_cast<uint8_t>(entry.sensorType),
            entry.name,
            entry.pins[0],
            entry.pins[1],
            entry.options2[0],
            entry.options2[1],
            entry.enabled ? 1 : 0);

        if (_commandMgrComputer)
            _commandMgrComputer->sendCommand(SensorConfigGetAll, buf);

        if (_commandMgrLink && _commandMgrLink != _commandMgrComputer)
            _commandMgrLink->sendCommand(SensorConfigGetAll, buf);
    }

public:
    SensorConfigCommandHandler(SerialCommandManager* commandMgrComputer,
                               SerialCommandManager* commandMgrLink)
        : _commandMgrComputer(commandMgrComputer),
          _commandMgrLink(commandMgrLink)
    {
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        static const char* cmds[] = {
            SensorConfigGetAll,
            SensorConfigAddUpdate,
            SensorConfigRemove,
            SensorConfigRename,
            SensorConfigSetPin,
            SensorConfigSetEnabled,
            SensorConfigSetOptions
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

        if (SystemFunctions::commandMatches(command, SensorConfigGetAll))
        {
            for (uint8_t i = 0; i < config->sensors.count && i < ConfigMaxSensors; i++)
            {
                broadcastSensorConfig(sender, i, config->sensors.sensors[i]);
            }

            sendAckOk(sender, command);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigAddUpdate))
        {
            // N1:i=<idx>;t=<type>;o0=<opt0>;o1=<opt1>
            uint8_t idx  = getParamValueU8t(params, paramCount, "i");
            uint8_t type = getParamValueU8t(params, paramCount, "t");
            int8_t opt0 = getParamValue8t(params, paramCount, "o0");
            int8_t opt1 = getParamValue8t(params, paramCount, "o1");

            if (idx >= ConfigMaxSensors)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            SensorEntry& entry = config->sensors.sensors[idx];
            entry.enabled    = true;
            entry.sensorType = static_cast<SensorIdList>(type);
            entry.options1[0] = opt0;
            entry.options1[1] = opt1;

            if (idx >= config->sensors.count)
                config->sensors.count = idx + 1;

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigRemove))
        {
            // N2:<idx>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing sensor index"));
                return true;
            }

            uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));

            if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            SensorEntry& entry = config->sensors.sensors[idx];
            entry.enabled = false;
            entry.sensorType = static_cast<SensorIdList>(0);
            entry.name[0] = '\0';
            memset(entry.pins, PinDisabled, sizeof(entry.pins));
            memset(entry.options1, 0, sizeof(entry.options1));
            memset(entry.options2, 0, sizeof(entry.options2));

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigRename))
        {
            // N3:<idx>=<name>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing parameters"));
                return true;
            }

            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));

            if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            if (params[0].value[0] == '\0')
            {
                sendAckErr(sender, command, F("Missing name"));
                return true;
            }

            SensorEntry& entry = config->sensors.sensors[idx];
            strncpy(entry.name, params[0].value, ConfigMaxSensorNameLength - 1);
            entry.name[ConfigMaxSensorNameLength - 1] = '\0';

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigSetPin))
        {
            // N4:<idx>;s=<slot>;v=<pin>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing parameters"));
                return true;
            }

            uint8_t idx = getParamValueU8t(params, paramCount, "i");
            uint8_t slot = getParamValueU8t(params, paramCount, "s");
            uint8_t pin = getParamValueU8t(params, paramCount, "v");

            if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            if (slot >= ConfigMaxSensorPins)
            {
                sendAckErr(sender, command, F("Invalid pin slot"));
                return true;
            }

            config->sensors.sensors[idx].pins[slot] = pin;

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigSetEnabled))
        {
            // N5:<idx>=<0|1>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing parameters"));
                return true;
            }

            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            bool enabled = SystemFunctions::parseBooleanValue(params[0].value);

            if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            config->sensors.sensors[idx].enabled = enabled;

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        if (SystemFunctions::commandMatches(command, SensorConfigSetOptions))
        {
            // N6:<idx>;s=<slot>;v=<value>
            if (paramCount < 1)
            {
                sendAckErr(sender, command, F("Missing parameters"));
                return true;
            }

            uint8_t idx = getParamValueU8t(params, paramCount, "i");
            uint8_t slot = getParamValueU8t(params, paramCount, "s");
            uint8_t option = getParamValueU8t(params, paramCount, "o");

            if (idx >= ConfigMaxSensors || idx >= config->sensors.count)
            {
                sendAckErr(sender, command, F("Invalid sensor index"));
                return true;
            }

            if (option > 1)
            {
                sendAckErr(sender, command, F("Invalid options group"));
                return true;
            }

            constexpr uint8_t options1Size = sizeof(SensorEntry::options1) / sizeof(SensorEntry::options1[0]);

            if (slot >= options1Size)
            {
                sendAckErr(sender, command, F("Invalid option slot"));
                return true;
            }

            if (option == 0)
            {
                // options1 are int8_t
                int8_t val8 = getParamValue8t(params, paramCount, "v");
                config->sensors.sensors[idx].options1[slot] = val8;
            }
            else
            {
                // options2 are int16_t and may exceed int8 range (e.g. 180 => 18.0)
                int16_t val16 = getParamValue16t(params, paramCount, "v");
                config->sensors.sensors[idx].options2[slot] = val16;
            }

            StringKeyValue reboot = makeParam("reboot", 1);
            sendAckOk(sender, command, &reboot);
            return true;
        }

        sendAckErr(sender, command, F("Unknown sensor config command"));
        return true;
    }
};
