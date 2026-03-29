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

#include "SchedulerCommandHandler.h"
#include "SystemFunctions.h"
//#include "SystemDefinitions.h"

SchedulerCommandHandler::SchedulerCommandHandler(RelayController* relayController)
    : _relayController(relayController)
{
}

const char* const* SchedulerCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] =
    {
        TimerListEvents, TimerGetEvent, TimerSetEvent,
        TimerDeleteEvent, TimerEnableEvent, TimerClearAll, TimerTriggerNow
    };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SchedulerCommandHandler::parseCompound(const char* value, uint8_t& type, uint8_t payload[ConfigSchedulerPayloadSize])
{
    char buf[DefaultMaxParamValueLength + 1];
    strncpy(buf, value, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* token = strtok(buf, ",");
    if (!token) return false;
    type = static_cast<uint8_t>(atoi(token));

    for (uint8_t i = 0; i < ConfigSchedulerPayloadSize; ++i)
    {
        token = strtok(nullptr, ",");
        if (!token) return false;
        payload[i] = static_cast<uint8_t>(atoi(token));
    }

    return true;
}

void SchedulerCommandHandler::buildCompound(char* buf, uint8_t bufLen, uint8_t type, const uint8_t payload[ConfigSchedulerPayloadSize])
{
    snprintf(buf, bufLen, "%u,%u,%u,%u,%u", type, payload[0], payload[1], payload[2], payload[3]);
}

void SchedulerCommandHandler::rebuildEventCount(Config* cfg)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
    {
        if (cfg->scheduler.events[i].triggerType != TriggerType::None ||
            cfg->scheduler.events[i].actionType != SchedulerActionType::None)
        {
            ++count;
        }
    }
    cfg->scheduler.eventCount = count;
}

bool SchedulerCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    Config* cfg = ConfigManager::getConfigPtr();
    if (!cfg)
    {
        sendAckErr(sender, command, F("Config not available"));
        return true;
    }

    // T0 — List all events: returns count and per-slot enabled states
    if (SystemFunctions::commandMatches(command, TimerListEvents))
    {
        char states[ConfigMaxScheduledEvents * 2] = { 0 };
        char* p = states;
        for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
        {
            if (i > 0) *p++ = ',';
            *p++ = cfg->scheduler.events[i].enabled ? '1' : '0';
        }
        *p = '\0';

        char msgBuf[DefaultMaxMessageLength];
        snprintf(msgBuf, sizeof(msgBuf), "count=%u;s=%s", cfg->scheduler.eventCount, states);
        sender->sendCommand(command, msgBuf);
        return true;
    }

    // T1 — Get event detail at index v=<index>
    if (SystemFunctions::commandMatches(command, TimerGetEvent))
    {
        if (paramCount < 1)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));
        if (idx >= ConfigMaxScheduledEvents)
        {
            sendAckErr(sender, command, F("Index out of range"), &params[0]);
            return true;
        }

        const ScheduledEvent& ev = cfg->scheduler.events[idx];
        if (ev.triggerType == TriggerType::None && ev.actionType == SchedulerActionType::None && !ev.enabled)
        {
            sendAckErr(sender, command, F("Slot is empty"), &params[0]);
            return true;
        }

        char tBuf[24], cBuf[24], aBuf[24];
        buildCompound(tBuf, sizeof(tBuf), static_cast<uint8_t>(ev.triggerType), ev.triggerPayload);
        buildCompound(cBuf, sizeof(cBuf), static_cast<uint8_t>(ev.conditionType), ev.conditionPayload);
        buildCompound(aBuf, sizeof(aBuf), static_cast<uint8_t>(ev.actionType), ev.actionPayload);

        char msgBuf[DefaultMaxMessageLength];
        snprintf(msgBuf, sizeof(msgBuf), "i=%u;e=%u;t=%s;c=%s;a=%s", idx, ev.enabled ? 1 : 0, tBuf, cBuf, aBuf);
        sender->sendCommand(command, msgBuf);
        return true;
    }

    // T2 — Set event: i=<index>;e=<enabled>;t=<type,b0,b1,b2,b3>;c=<type,b0,b1,b2,b3>;a=<type,b0,b1,b2,b3>
    if (SystemFunctions::commandMatches(command, TimerSetEvent))
    {
        if (paramCount < 5)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        uint8_t idx = 0xFF, enabled = 0;
        uint8_t trigType = 0, condType = 0, actType = 0;
        uint8_t trigPayload[ConfigSchedulerPayloadSize] = { 0 };
        uint8_t condPayload[ConfigSchedulerPayloadSize] = { 0 };
        uint8_t actPayload[ConfigSchedulerPayloadSize] = { 0 };
        bool hasI = false, hasE = false, hasT = false, hasC = false, hasA = false;

        for (uint8_t i = 0; i < paramCount; ++i)
        {
            if (strcmp(params[i].key, "i") == 0)
            {
                idx = static_cast<uint8_t>(atoi(params[i].value));
                hasI = true;
            }
            else if (strcmp(params[i].key, "e") == 0)
            {
                enabled = static_cast<uint8_t>(atoi(params[i].value));
                hasE = true;
            }
            else if (strcmp(params[i].key, "t") == 0)
            {
                hasT = parseCompound(params[i].value, trigType, trigPayload);
            }
            else if (strcmp(params[i].key, "c") == 0)
            {
                hasC = parseCompound(params[i].value, condType, condPayload);
            }
            else if (strcmp(params[i].key, "a") == 0)
            {
                hasA = parseCompound(params[i].value, actType, actPayload);
            }
        }

        if (!hasI || !hasE || !hasT || !hasC || !hasA)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        if (idx >= ConfigMaxScheduledEvents)
        {
            sendAckErr(sender, command, F("Index out of range"));
            return true;
        }

        if (trigType > static_cast<uint8_t>(TriggerType::Date))
        {
            sendAckErr(sender, command, F("Invalid trigger type"));
            return true;
        }

        if (condType > static_cast<uint8_t>(ConditionType::RelayIsOff))
        {
            sendAckErr(sender, command, F("Invalid condition type"));
            return true;
        }

        if (actType > static_cast<uint8_t>(SchedulerActionType::AllRelaysOff))
        {
            sendAckErr(sender, command, F("Invalid action type"));
            return true;
        }

        ScheduledEvent& ev = cfg->scheduler.events[idx];
        ev.enabled = enabled > 0;
        ev.triggerType = static_cast<TriggerType>(trigType);
        memcpy(ev.triggerPayload, trigPayload, ConfigSchedulerPayloadSize);
        ev.conditionType = static_cast<ConditionType>(condType);
        memcpy(ev.conditionPayload, condPayload, ConfigSchedulerPayloadSize);
        ev.actionType = static_cast<SchedulerActionType>(actType);
        memcpy(ev.actionPayload, actPayload, ConfigSchedulerPayloadSize);
        rebuildEventCount(cfg);

        sendAckOk(sender, command);
        return true;
    }

    // T3 — Delete event at index v=<index>
    if (SystemFunctions::commandMatches(command, TimerDeleteEvent))
    {
        if (paramCount < 1)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));
        if (idx >= ConfigMaxScheduledEvents)
        {
            sendAckErr(sender, command, F("Index out of range"), &params[0]);
            return true;
        }

        memset(&cfg->scheduler.events[idx], 0x00, sizeof(ScheduledEvent));
        rebuildEventCount(cfg);

        sendAckOk(sender, command, &params[0]);
        return true;
    }

    // T4 — Enable/disable event: i=<index>;v=<0|1>
    if (SystemFunctions::commandMatches(command, TimerEnableEvent))
    {
        if (paramCount < 2)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        uint8_t idx = 0xFF, enabled = 0;
        bool hasI = false, hasV = false;

        for (uint8_t i = 0; i < paramCount; ++i)
        {
            if (strcmp(params[i].key, "i") == 0)
            {
                idx = static_cast<uint8_t>(atoi(params[i].value));
                hasI = true;
            }
            else if (strcmp(params[i].key, "v") == 0)
            {
                enabled = static_cast<uint8_t>(atoi(params[i].value));
                hasV = true;
            }
        }

        if (!hasI || !hasV)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        if (idx >= ConfigMaxScheduledEvents)
        {
            sendAckErr(sender, command, F("Index out of range"));
            return true;
        }

        if (enabled > 1)
        {
            sendAckErr(sender, command, F("Invalid enabled value (0 or 1)"));
            return true;
        }

        cfg->scheduler.events[idx].enabled = enabled > 0;
        sendAckOk(sender, command);
        return true;
    }

    // T5 — Clear all events
    if (SystemFunctions::commandMatches(command, TimerClearAll))
    {
        memset(&cfg->scheduler, 0x00, sizeof(SchedulerSettings));
        sendAckOk(sender, command);
        return true;
    }

    // T6 — Trigger event now (bypasses trigger and condition, executes action immediately)
    if (SystemFunctions::commandMatches(command, TimerTriggerNow))
    {
        if (paramCount < 1)
        {
            sendAckErr(sender, command, F("Missing params"));
            return true;
        }

        uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));
        if (idx >= ConfigMaxScheduledEvents)
        {
            sendAckErr(sender, command, F("Index out of range"), &params[0]);
            return true;
        }

        if (!executeAction(sender, command, cfg->scheduler.events[idx]))
        {
            return true;
        }

        sendAckOk(sender, command, &params[0]);
        return true;
    }

    sendAckErr(sender, command, F("Unknown timer command"));
    return true;
}

bool SchedulerCommandHandler::executeAction(SerialCommandManager* sender, const char* command, const ScheduledEvent& event)
{
    if (!_relayController)
    {
        sendAckErr(sender, command, F("Relay controller not available"));
        return false;
    }

    switch (event.actionType)
    {
        case SchedulerActionType::RelayOn:
        {
            CommandResult result = _relayController->setRelayState(event.actionPayload[0], true);
            if (!result.success)
            {
                sendAckErr(sender, command, F("Relay operation failed"));
                return false;
            }
            return true;
        }

        case SchedulerActionType::RelayOff:
        {
            CommandResult result = _relayController->setRelayState(event.actionPayload[0], false);
            if (!result.success)
            {
                sendAckErr(sender, command, F("Relay operation failed"));
                return false;
            }
            return true;
        }

        case SchedulerActionType::RelayToggle:
        {
            CommandResult current = _relayController->getRelayStatus(event.actionPayload[0]);
            if (current.status == DefaultValue)
            {
                sendAckErr(sender, command, F("Relay operation failed"));
                return false;
            }
            CommandResult result = _relayController->setRelayState(event.actionPayload[0], current.status == 0);
            if (!result.success)
            {
                sendAckErr(sender, command, F("Relay operation failed"));
                return false;
            }
            return true;
        }

        case SchedulerActionType::RelayPulse:
        {
            // Pulse duration is managed by the runtime scheduler; T6 performs the on transition only
            CommandResult result = _relayController->setRelayState(event.actionPayload[0], true);
            if (!result.success)
            {
                sendAckErr(sender, command, F("Relay operation failed"));
                return false;
            }
            return true;
        }

        case SchedulerActionType::AllRelaysOn:
            _relayController->turnAllRelaysOn();
            return true;

        case SchedulerActionType::AllRelaysOff:
            _relayController->turnAllRelaysOff();
            return true;

        case SchedulerActionType::None:
        default:
            sendAckErr(sender, command, F("No action defined"));
            return false;
    }
}
