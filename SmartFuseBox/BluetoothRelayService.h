#pragma once

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

    explicit BluetoothRelayService(RelayCommandHandler* relayHandler);
    ~BluetoothRelayService() override;

    // BluetoothServiceBase overrides
    bool begin() override;
    const char* getServiceUUID() const override;
    const char* getServiceName() const override;
    void loop(unsigned long currentMillis) override;
    uint8_t getCharacteristicCount() const override;
    void* getBLEService() override;

    // Notify all relay states to connected centrals
    void notifyAll();

private:
    // Dependencies
    RelayCommandHandler* _relayHandler;
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