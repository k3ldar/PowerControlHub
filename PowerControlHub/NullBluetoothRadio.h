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

#include "IBluetoothRadio.h"

class SystemCommandHandler;
class SensorCommandHandler;
class RelayController;
class WarningManager;
class SerialCommandManager;

// Null implementation of IBluetoothRadio for boards without Bluetooth hardware.
// All methods return safe defaults or failure states.
// Allows Bluetooth subsystem code to compile and gracefully handle missing hardware at runtime.
class NullBluetoothRadio : public IBluetoothRadio
{
public:
    NullBluetoothRadio(SystemCommandHandler* systemHandler = nullptr,
                       SensorCommandHandler* sensorHandler = nullptr,
                       RelayController* relayController = nullptr,
                       WarningManager* warningManager = nullptr,
                       SerialCommandManager* commandMgrComputer = nullptr)
    {
        (void)systemHandler;
        (void)sensorHandler;
        (void)relayController;
        (void)warningManager;
        (void)commandMgrComputer;
    }

    bool isEnabled() const override
    {
        return false;
    }

    bool setEnabled(bool enabled) override
    {
        (void)enabled;
        return false;
    }

    void applyConfig(const Config* cfg) override
    {
        (void)cfg;
    }

    void loop() override
    {
    }

    void advertise() override
    {
    }

    void stopAdvertise() override
    {
    }

    void setLocalName(const char* name) override
    {
        (void)name;
    }

    void setDeviceName(const char* name) override
    {
        (void)name;
    }

    void poll() override
    {
    }

    bool isConnected() const override
    {
        return false;
    }

    void setAdvertisedServiceUUID(const char* uuid) override
    {
        (void)uuid;
    }

    void setAdvertisedService(void* service) override
    {
        (void)service;
    }
};
