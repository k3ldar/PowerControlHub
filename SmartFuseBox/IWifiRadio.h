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

#include <Arduino.h>
#include "SystemDefinitions.h"

class IWifiClient;

class IWifiRadio
{
public:
    virtual ~IWifiRadio() = default;

    virtual bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) = 0;

    virtual bool beginClient(
        const char* ssid,
        const char* password) = 0;

    virtual void disconnect() = 0;
    virtual void end() = 0;
    virtual WifiConnectionState status() = 0;
    virtual int32_t rssi() = 0;
    virtual IPAddress localIP() = 0;
    virtual bool hasModule() = 0;

    virtual void beginServer(uint16_t port) = 0;
    virtual void endServer() = 0;
    virtual IWifiClient* available() = 0;
    virtual IWifiClient* createClient() = 0;
};