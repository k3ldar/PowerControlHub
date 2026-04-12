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
// 
// 
// 

#include "Local.h"

#if defined(BLUETOOTH_SUPPORT)

#include "Local.h"

#if defined(BLUETOOTH_SUPPORT)

#include "BluetoothSensorService.h"
#include "SystemFunctions.h"
#include "SystemCpuMonitor.h"

BluetoothSensorService::BluetoothSensorService(SensorCommandHandler* commandHandler)
    : _commandHandler(commandHandler),
    _service(nullptr),
    _charTemperatureUsage(nullptr),
    _charHumidityUsage(nullptr),
    _charBearingUsage(nullptr),
    _charCompassTemperatureUsage(nullptr),
    _charSpeedUsage(nullptr),
    _charWaterLevelUsage(nullptr),
    _charWaterPumpActive(nullptr),
	_lastUpdate(0)
{
}

BluetoothSensorService::~BluetoothSensorService()
{
    delete _service;
    delete _charTemperatureUsage;
    delete _charHumidityUsage;
    delete _charBearingUsage;
    delete _charCompassTemperatureUsage;
    delete _charSpeedUsage;
    delete _charWaterLevelUsage;
    delete _charWaterPumpActive;
}

bool BluetoothSensorService::begin()
{
    if (!_commandHandler)
    {
        return false;
    }

    // Create the service
    _service = new BLEService(SensorServiceUuid);

    if (!_service)
    {
        return false;
    }

    // Create characteristics with ArduinoBLE typed characteristics
    _charTemperatureUsage = new BLEFloatCharacteristic(TemperatureUsageUuid, BLERead | BLENotify);
    _charHumidityUsage = new BLEFloatCharacteristic(HumidityUsageUuid, BLERead | BLENotify);
    _charBearingUsage = new BLEFloatCharacteristic(BearingUsageUuid, BLERead | BLENotify);
    _charCompassTemperatureUsage = new BLEFloatCharacteristic(CompasTempUsageUuid, BLERead | BLENotify);
	_charSpeedUsage = new BLEIntCharacteristic(SpeedUsageUuid, BLERead | BLENotify);
	_charWaterLevelUsage = new BLEIntCharacteristic(WaterLevelUsageUuid, BLERead | BLENotify);
	_charWaterPumpActive = new BLEByteCharacteristic(WaterPumpUsageUuid, BLERead | BLENotify);

    // Add characteristics to service
    _service->addCharacteristic(*_charTemperatureUsage);
    _service->addCharacteristic(*_charHumidityUsage);
    _service->addCharacteristic(*_charBearingUsage);
    _service->addCharacteristic(*_charCompassTemperatureUsage);
    _service->addCharacteristic(*_charSpeedUsage);
    _service->addCharacteristic(*_charWaterLevelUsage);
    _service->addCharacteristic(*_charWaterPumpActive);

    // Initialize values
    updateAll();

    // Add service to BLE
    BLE.addService(*_service);

    return true;
}

const char* BluetoothSensorService::getServiceUUID() const
{
    return SensorServiceUuid;
}

const char* BluetoothSensorService::getServiceName() const
{
    return "Sensors";
}

void* BluetoothSensorService::getBLEService()
{
    return _service;
}

void BluetoothSensorService::loop(uint64_t currentMillis)
{
    if (SystemFunctions::hasElapsed(currentMillis, _lastUpdate, UpdateIntervalMs))
    {
        updateAll();
        _lastUpdate = currentMillis;
    }
}

uint8_t BluetoothSensorService::getCharacteristicCount() const
{
    return 7;
}

void BluetoothSensorService::updateAll()
{
    updateTemperature();
    updateHumidity();
    updateBearing();
    updateCompassTemperature();
    updateSpeed();
    updateWaterLevel();
    updateWaterPumpActive();
}

void BluetoothSensorService::updateTemperature()
{
    if (_charTemperatureUsage)
    {
        float temperature = _commandHandler->getTemperature();

        if (!isnan(temperature))
        {
            _charTemperatureUsage->writeValue(temperature);
        }
	}
}

void BluetoothSensorService::updateHumidity()
{
    if (_charHumidityUsage)
    {
        float humidity = _commandHandler->getHumidity();

        if (!isnan(humidity))
        {
            _charHumidityUsage->writeValue(humidity);
        }
	}
}

void BluetoothSensorService::updateBearing()
{
    if (_charBearingUsage)
    {
        float bearing = _commandHandler->getBearing();

        if (!isnan(bearing))
        {
            _charBearingUsage->writeValue(bearing);
        }
    }
}

void BluetoothSensorService::updateCompassTemperature()
{
    if (_charCompassTemperatureUsage)
    {
        float compassTemp = _commandHandler->getCompassTemperature();

        if (!isnan(compassTemp))
        {
            _charCompassTemperatureUsage->writeValue(compassTemp);
        }
    }
}

void BluetoothSensorService::updateSpeed()
{
    if (_charSpeedUsage)
    {
        uint8_t speed = _commandHandler->getSpeed();
        _charSpeedUsage->writeValue(speed);
    }
}

void BluetoothSensorService::updateWaterLevel()
{
    if (_charWaterLevelUsage)
    {
        uint16_t waterLevel = static_cast<uint16_t>(_commandHandler->getWaterLevel());
        _charWaterLevelUsage->writeValue(waterLevel);
	}
}

void BluetoothSensorService::updateWaterPumpActive()
{
    if (_charWaterPumpActive)
    {
		bool isActive = _commandHandler->getWaterPumpActive();
		_charWaterPumpActive->writeValue((uint8_t)(isActive ? 1 : 0));
	}
}

#endif

#endif

