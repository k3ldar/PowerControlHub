
#pragma once

#include "Config.h"

#if defined(SCHEDULER_SUPPORT)

#include "INetworkCommandHandler.h"
#include "SystemDefinitions.h"

class SchedulerNetworkHandler : public INetworkCommandHandler
{
public:
    SchedulerNetworkHandler() = default;

    const char* getRoute() const override { return "/api/timer"; }

    void formatWifiStatusJson(IWifiClient* client) override;

    void formatStatusJson(char* buffer, size_t size);

    CommandResult handleRequest(const char* method,
        const char* command,
        StringKeyValue* params,
        uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) override;

private:
    static void formatEventJson(char* buffer, size_t size, uint8_t idx, const ScheduledEvent& ev);
};

#endif
