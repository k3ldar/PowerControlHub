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

#include "SystemDefinitions.h"

// Forward declarations to avoid heavy includes
struct Config;
class WifiServer;
class INetworkCommandHandler;
class NetworkJsonVisitor;

// Lightweight, platform-agnostic WiFi controller interface.
// Keep this header free of Arduino or platform headers so it compiles everywhere.
class IWifiController
{
public:
    virtual ~IWifiController() = default;

    // Return whether the controller is currently enabled.
    virtual bool isEnabled() const = 0;

    // Enable/disable the controller; return true on success.
    virtual bool setEnabled(bool enabled) = 0;

    // Apply configuration (may be called at runtime).
    virtual void applyConfig(const Config* config) = 0;

    // Per-loop processing called from the app loop.
    virtual void update(uint64_t currentMillis) = 0;

    // Register network command handlers (for dependency injection).
    virtual void registerHandlers(INetworkCommandHandler** handlers, size_t count) = 0;

    // Register JSON status visitors (for status reporting).
    virtual void registerJsonVisitors(NetworkJsonVisitor** visitors, uint8_t count) = 0;

    // Get the underlying server instance (nullable) - use sparingly, prefer interface methods.
    virtual WifiServer* getServer() = 0;

    // Get connection state without accessing WifiServer directly.
    virtual WifiConnectionState getConnectionState() const = 0;

    // Get IP address without accessing WifiServer directly.
    virtual void getIpAddress(char* buffer, size_t bufferSize) const = 0;
};
