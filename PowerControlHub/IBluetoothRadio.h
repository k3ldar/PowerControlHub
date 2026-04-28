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

// Forward declare Config to avoid heavy includes
struct Config;

// Lightweight, platform-agnostic Bluetooth radio interface.
// Keep this header free of Arduino or other platform headers so it compiles everywhere.
class IBluetoothRadio
{
public:
    virtual ~IBluetoothRadio() = default;

    // Return whether the radio is currently enabled.
    virtual bool isEnabled() const = 0;

    // Enable/disable the radio; return true on success.
    virtual bool setEnabled(bool enabled) = 0;

    // Apply configuration (may be called at runtime).
    virtual void applyConfig(const Config* cfg) = 0;

    // Per-loop processing called from the app loop.
    virtual void loop() = 0;

    // Start advertising BLE services.
    virtual void advertise() = 0;

    // Stop advertising BLE services.
    virtual void stopAdvertise() = 0;

    // Set the local device name.
    virtual void setLocalName(const char* name) = 0;

    // Set the device name.
    virtual void setDeviceName(const char* name) = 0;

    // Poll for BLE events.
    virtual void poll() = 0;

    // Check if a client is currently connected.
    virtual bool isConnected() const = 0;

    // Set an advertised service by UUID.
    virtual void setAdvertisedServiceUUID(const char* uuid) = 0;

    // Set an advertised service (platform-specific service object as void*).
    virtual void setAdvertisedService(void* service) = 0;
};

