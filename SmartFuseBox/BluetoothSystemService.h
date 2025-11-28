#pragma once

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
    void loop(unsigned long currentMillis) override;
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

    unsigned long _lastHeartbeat;
    unsigned long _lastMemoryUpdate;
    unsigned long _lastCpuUpdate;
    
    static constexpr unsigned long HEARTBEAT_INTERVAL_MS = 1000;
    static constexpr unsigned long MEMORY_UPDATE_INTERVAL_MS = 5000;
    static constexpr unsigned long CPU_UPDATE_INTERVAL_MS = 2000;

    void updateFreeMemory();
    void updateCpuUsage();
    void sendHeartbeat();
};