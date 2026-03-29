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
#include "BaseCommandHandler.h"
#include "ConfigManager.h"
#include "RelayController.h"

constexpr uint8_t RelayImageButtonColorBlue   = 2;
constexpr uint8_t RelayImageButtonColorYellow = 7;

// internal message handlers
class RelayCommandHandler : public BaseCommandHandler
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;
    RelayController* _relayController;
    Config* _config;

    void broadcastRelayStatus(const char* cmd, const StringKeyValue* param = nullptr);
    void sendAllRelayConfig(SerialCommandManager* sender);

public:
    RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, RelayController* relayController);
    ~RelayCommandHandler();
    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;

    const char* const* supportedCommands(size_t& count) const override;

    void setup();
    void configUpdated(Config* config);

    uint8_t getRelayCount() const { return _relayController ? _relayController->getRelayCount() : 0; }
};

