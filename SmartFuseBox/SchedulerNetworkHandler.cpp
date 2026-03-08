
#include "Local.h"
#include "SchedulerNetworkHandler.h"

#if defined(SCHEDULER_SUPPORT)

#include "ConfigManager.h"
#include "SystemDefinitions.h"

CommandResult SchedulerNetworkHandler::handleRequest(const char* method,
    const char* command,
    StringKeyValue* params,
    uint8_t paramCount,
    char* responseBuffer,
    size_t bufferSize)
{
    (void)method;

    Config* cfg = ConfigManager::getConfigPtr();
    if (!cfg)
    {
        formatJsonResponse(responseBuffer, bufferSize, false, "Config not available");
        return CommandResult::error(InvalidConfiguration);
    }

    // T0 — List events: returns count and per-slot enabled states
    if (strcmp(command, TimerListEvents) == 0 || command[0] == '\0')
    {
        int written = snprintf(responseBuffer, bufferSize,
            "\"success\":true,\"schedule\":{\"count\":%u,\"slots\":[",
            cfg->scheduler.eventCount);

        for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
        {
            int n = snprintf(responseBuffer + written, bufferSize - written,
                "%s%u",
                i > 0 ? "," : "",
                cfg->scheduler.events[i].enabled ? 1 : 0);

            if (n < 0 || written + n >= static_cast<int>(bufferSize))
                break;

            written += n;
        }

        snprintf(responseBuffer + written, bufferSize - written, "]}");
        return CommandResult::ok();
    }

    // T1 — Get event detail at index v=<index>
    if (strcmp(command, TimerGetEvent) == 0)
    {
        if (paramCount < 1)
        {
            formatJsonResponse(responseBuffer, bufferSize, false, "Missing params");
            return CommandResult::error(InvalidCommandParameters);
        }

        uint8_t idx = static_cast<uint8_t>(atoi(params[0].value));
        if (idx >= ConfigMaxScheduledEvents)
        {
            formatJsonResponse(responseBuffer, bufferSize, false, "Index out of range");
            return CommandResult::error(InvalidCommandParameters);
        }

        const ScheduledEvent& ev = cfg->scheduler.events[idx];
        int written = snprintf(responseBuffer, bufferSize, "\"success\":true,\"event\":");

        if (written > 0 && written < static_cast<int>(bufferSize))
        {
            formatEventJson(responseBuffer + written, bufferSize - written, idx, ev);
        }

        return CommandResult::ok();
    }

    formatJsonResponse(responseBuffer, bufferSize, false, "Unknown timer command");
    return CommandResult::error(InvalidCommandParameters);
}

void SchedulerNetworkHandler::formatEventJson(char* buffer, size_t size, uint8_t idx, const ScheduledEvent& ev)
{
    snprintf(buffer, size,
        "{\"i\":%u,\"enabled\":%u,"
        "\"trigger\":{\"type\":%u,\"b\":[%u,%u,%u,%u]},"
        "\"condition\":{\"type\":%u,\"b\":[%u,%u,%u,%u]},"
        "\"action\":{\"type\":%u,\"b\":[%u,%u,%u,%u]}}",
        idx,
        ev.enabled ? 1u : 0u,
        static_cast<uint8_t>(ev.triggerType),
        ev.triggerPayload[0], ev.triggerPayload[1], ev.triggerPayload[2], ev.triggerPayload[3],
        static_cast<uint8_t>(ev.conditionType),
        ev.conditionPayload[0], ev.conditionPayload[1], ev.conditionPayload[2], ev.conditionPayload[3],
        static_cast<uint8_t>(ev.actionType),
        ev.actionPayload[0], ev.actionPayload[1], ev.actionPayload[2], ev.actionPayload[3]);
}

void SchedulerNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
    if (!buffer || size == 0)
        return;

    Config* cfg = ConfigManager::getConfigPtr();
    if (!cfg)
    {
        snprintf(buffer, size, "\"schedule\":{}");
        return;
    }

    int written = snprintf(buffer, size,
        "\"schedule\":{\"count\":%u,\"slots\":[",
        cfg->scheduler.eventCount);

    for (uint8_t i = 0; i < ConfigMaxScheduledEvents; ++i)
    {
        int n = snprintf(buffer + written, size - written,
            "%s%u",
            i > 0 ? "," : "",
            cfg->scheduler.events[i].enabled ? 1 : 0);

        if (n < 0 || written + n >= static_cast<int>(size))
            break;

        written += n;
    }

    snprintf(buffer + written, size - written, "]}");
}

void SchedulerNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
    char buffer[MaximumJsonResponseBufferSize];
    buffer[0] = '\0';

    formatStatusJson(buffer, sizeof(buffer));
    client->print(buffer);
}

#endif
