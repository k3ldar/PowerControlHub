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

#include "INetworkCommandHandler.h"
#include "BaseConfigCommandHandler.h"

/**
 * @brief Network handler for remote/external sensor configuration (E-series) via /api/externalsensor.
 *
 * Exposes the same E0–E4 operations as ExternalSensorConfigCommandHandler but over
 * the WiFi REST interface. The GET route returns a JSON status block containing all
 * configured external sensors.
 */
class ExternalSensorNetworkHandler : public INetworkCommandHandler, public BaseConfigCommandHandler
{
public:
    ExternalSensorNetworkHandler() = default;

    const char* getRoute() const override { return "/api/externalsensor"; }

    void formatWifiStatusJson(IWifiClient* client) override;

    void formatStatusJson(IWifiClient* client);

    CommandResult handleRequest(const char* method,
        const char* command,
        StringKeyValue* params,
        uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) override;
};
