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

#include "IWifiRadio.h"
#include "NullWifiClient.h"

// Null implementation of IWifiRadio for boards without WiFi hardware.
// All methods return safe defaults or failure states.
// Allows WiFi subsystem code to compile and gracefully handle missing hardware at runtime.
class NullWifiRadio : public IWifiRadio
{
public:
    bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) override
    {
		(void)ssid;
        (void)password;
		(void)ip;
		(void)subnet;
        return false;
    }

    bool beginClient(
        const char* ssid,
        const char* password) override
    {
        (void)ssid;
		(void)password;
        return false;
    }

    void disconnect() override
    {
    }

    void end() override
    {
    }

    WifiConnectionState status() override
    {
        return WifiConnectionState::Disconnected;
    }

    int32_t rssi() override
    {
        return 0;
    }

    IPAddress localIP() override
    {
        return IPAddress(0, 0, 0, 0);
    }

    bool hasModule() override
    {
        return false;
    }

    void beginServer(uint16_t port) override
    {
        (void)port;
    }

    void endServer() override
    {
    }

    IWifiClient* available() override
    {
        return nullptr;
    }

    IWifiClient* createClient() override
    {
        return new NullWifiClient();
    }
};
