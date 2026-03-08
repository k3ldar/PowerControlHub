#pragma once

#include "Config.h"

#if defined(SCHEDULER_SUPPORT)

#include "BaseCommandHandler.h"
#include "ConfigManager.h"
#include "RelayController.h"
#include "SystemDefinitions.h"

class SchedulerCommandHandler : public BaseCommandHandler
{
private:
    RelayController* _relayController;

    // Parses "type,b0,b1,b2,b3" into type and payload bytes. Returns false on malformed input.
    static bool parseCompound(const char* value, uint8_t& type, uint8_t payload[ConfigSchedulerPayloadSize]);

    // Builds "type,b0,b1,b2,b3" into buf for sending responses.
    static void buildCompound(char* buf, uint8_t bufLen, uint8_t type, const uint8_t payload[ConfigSchedulerPayloadSize]);

    // Immediately executes the action of an event (used by T6). Returns false and sends error on failure.
    bool executeAction(SerialCommandManager* sender, const char* command, const ScheduledEvent& event);

    // Recounts configured event slots and updates cfg->scheduler.eventCount.
    static void rebuildEventCount(Config* cfg);

public:
    explicit SchedulerCommandHandler(RelayController* relayController);

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;
};

#endif
