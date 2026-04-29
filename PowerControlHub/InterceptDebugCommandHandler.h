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

#include <SerialCommandManager.h>
#include "BroadcastManager.h"

class InterceptDebugHandler : public ISerialCommandHandler
{
private:
    BroadcastManager* _broadcastManager;
public:
    InterceptDebugHandler(BroadcastManager* broadcastManager)
        : _broadcastManager(broadcastManager)
    {
    }

    bool supportsCommand(const char* command) const override
    {
		(void)command;

        // This handler intercepts all commands for debugging purposes
        return true;
    }

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override
    {
		(void)command;
		(void)params;
		(void)paramCount;
        _broadcastManager->getComputerSerial()->sendCommand(sender->getRawMessage(), "");
		return false; // Indicate that we did not fully handle the command
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        // Return empty array since we override supportsCommand() to claim all commands
        count = 0;
        return nullptr;
    }
};