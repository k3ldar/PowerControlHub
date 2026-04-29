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
#pragma once

#include "Config.h"

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
