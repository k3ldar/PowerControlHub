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
#include "RelayCommandHandler.h"

class BluetoothRelayService : public BluetoothServiceBase
{
public:
    // UUIDs (keep consistent with existing project usage)
    static constexpr const char* RelayServiceUuid     = "4fafc201-1fb5-459e-2fcc-c5c9c331914b";
    static constexpr const char* RelayStatesCharUuid  = "9b9f0002-3c4a-4d9a-a6c1-11aa22bb33cc";
    static constexpr const char* RelaySetCharUuid     = "9b9f0003-3c4a-4d9a-a6c1-11aa22bb33cc";

    explicit BluetoothRelayService(RelayController* relayController);
    ~BluetoothRelayService() override;

    // BluetoothServiceBase overrides
    bool begin() override;
    const char* getServiceUUID() const override;
    const char* getServiceName() const override;
    void loop(uint64_t currentMillis) override;
    uint8_t getCharacteristicCount() const override;
    void* getBLEService() override;

    // Notify all relay states to connected centrals
    void notifyAll();

private:
    // Dependencies
    RelayController* _relayController;
    uint8_t _relayCount;

    // BLE objects (manually managed)
    BLEService* _service;
    BLECharacteristic* _statesChar; // length = _relayCount, values 0/1 per relay
    BLECharacteristic* _setChar;    // length = 2 bytes: [idx, state]

    // Helpers
    void onWriteSet();
    void loadStates(uint8_t* buffer);

    // Static thunk + instance pointer (required by ArduinoBLE C-style callback)
    static BluetoothRelayService* _serviceInstance;
    static void onRelaySetWritten(BLEDevice central, BLECharacteristic characteristic);
};

#endif