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
#include "Local.h"

#if defined(BLUETOOTH_SUPPORT)

#include "BluetoothSystemService.h"
#include "SystemFunctions.h"
#include "SystemCpuMonitor.h"

BluetoothSystemService::BluetoothSystemService(SystemCommandHandler* commandHandler)
    : _commandHandler(commandHandler),
      _service(nullptr),
      _charHeartbeat(nullptr),
      _charInitialized(nullptr),
      _charFreeMemory(nullptr),
      _charCpuUsage(nullptr),
      _lastHeartbeat(0),
      _lastMemoryUpdate(0),
      _lastCpuUpdate(0)
{
}

BluetoothSystemService::~BluetoothSystemService()
{
    delete _service;
    delete _charHeartbeat;
    delete _charInitialized;
    delete _charFreeMemory;
    delete _charCpuUsage;
}

bool BluetoothSystemService::begin()
{
    if (!_commandHandler)
    {
        return false;
    }

    // Create the service
    _service = new BLEService(ServiceUuid);

    if (!_service)
    {
        return false;
    }

    // Create characteristics with ArduinoBLE typed characteristics
    _charHeartbeat = new BLEUnsignedIntCharacteristic(HeartbeatUuid, BLERead | BLENotify);
    _charInitialized = new BLEByteCharacteristic(InitializedUuid, BLERead | BLENotify);
    _charFreeMemory = new BLEUnsignedIntCharacteristic(FreeMemoryUuid, BLERead | BLENotify);
    _charCpuUsage = new BLEFloatCharacteristic(CpuUsageUuid, BLERead | BLENotify);

    // Add characteristics to service
    _service->addCharacteristic(*_charHeartbeat);
    _service->addCharacteristic(*_charInitialized);
    _service->addCharacteristic(*_charFreeMemory);
    _service->addCharacteristic(*_charCpuUsage);

    // Initialize values
    _charHeartbeat->writeValue((uint32_t)0xFFFF);
    _charInitialized->writeValue((uint8_t)0xFF);
    updateFreeMemory();
    updateCpuUsage();

    // Add service to BLE
    BLE.addService(*_service);

    return true;
}

const char* BluetoothSystemService::getServiceUUID() const
{
    return ServiceUuid;
}

const char* BluetoothSystemService::getServiceName() const
{
    return "System";
}

void* BluetoothSystemService::getBLEService()
{
    return _service;
}

void BluetoothSystemService::loop(unsigned long currentMillis)
{
    if (currentMillis - _lastHeartbeat >= HEARTBEAT_INTERVAL_MS)
    {
        sendHeartbeat();
        _lastHeartbeat = currentMillis;
    }

    if (currentMillis - _lastMemoryUpdate >= MEMORY_UPDATE_INTERVAL_MS)
    {
        updateFreeMemory();
        _lastMemoryUpdate = currentMillis;
    }

    if (currentMillis - _lastCpuUpdate >= CPU_UPDATE_INTERVAL_MS)
    {
        updateCpuUsage();
        _lastCpuUpdate = currentMillis;
    }
}

uint8_t BluetoothSystemService::getCharacteristicCount() const
{
    return 4;
}

void BluetoothSystemService::notifyInitialized()
{
    if (_charInitialized)
    {
        _charInitialized->writeValue((uint8_t)1);
    }
}

void BluetoothSystemService::updateFreeMemory()
{
    if (_charFreeMemory)
    {
        uint32_t freeMemory = SystemFunctions::freeMemory();
        _charFreeMemory->writeValue(freeMemory);
    }
}

void BluetoothSystemService::updateCpuUsage()
{
    if (_charCpuUsage)
    {
        float cpuUsage = SystemCpuMonitor::getCpuUsage();
        _charCpuUsage->writeValue(cpuUsage);
    }
}

void BluetoothSystemService::sendHeartbeat()
{
    if (_charHeartbeat)
    {
        static uint32_t heartbeatCount = 0;
        heartbeatCount++;
        _charHeartbeat->writeValue(heartbeatCount);
    }
}

#endif