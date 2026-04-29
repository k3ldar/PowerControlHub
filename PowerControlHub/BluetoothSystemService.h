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

#include "Local.h"

#if defined(BLUETOOTH_SUPPORT)

#include <Arduino.h>
#include <ArduinoBLE.h>
#include "BluetoothServiceBase.h"
#include "SystemCommandHandler.h"

class BluetoothSystemService : public BluetoothServiceBase
{
public:
    static constexpr const char* ServiceUuid = "4fafc201-1fb5-459e-0fcc-c5c9c331914b";
    
    static constexpr const char* HeartbeatUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a0";
    static constexpr const char* InitializedUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a1";
    static constexpr const char* FreeMemoryUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a2";
    static constexpr const char* CpuUsageUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a3";

    explicit BluetoothSystemService(SystemCommandHandler* commandHandler);
    ~BluetoothSystemService() override;

    bool begin() override;
    const char* getServiceUUID() const override;
    const char* getServiceName() const override;
    void loop(uint64_t currentMillis) override;
    uint8_t getCharacteristicCount() const override;
    void* getBLEService() override;

    void notifyInitialized();

private:
    SystemCommandHandler* _commandHandler;
    BLEService* _service;
    BLEUnsignedIntCharacteristic* _charHeartbeat;
    BLEByteCharacteristic* _charInitialized;
    BLEUnsignedIntCharacteristic* _charFreeMemory;
    BLEFloatCharacteristic* _charCpuUsage;

    uint64_t _lastHeartbeat;
    uint64_t _lastMemoryUpdate;
    uint64_t _lastCpuUpdate;
    
    static constexpr uint64_t HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr uint64_t MEMORY_UPDATE_INTERVAL_MS = 5000;
    static constexpr uint64_t CPU_UPDATE_INTERVAL_MS = 2000;

    void updateFreeMemory();
    void updateCpuUsage();
    void sendHeartbeat();
};

#endif