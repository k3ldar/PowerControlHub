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

#include "Local.h"

#if defined(BLUETOOTH_SUPPORT)

#include <Arduino.h>
#include <ArduinoBLE.h>
#include "BluetoothServiceBase.h"
#include "SystemCommandHandler.h"
#include "SensorCommandHandler.h"


class BluetoothSensorService : public BluetoothServiceBase
{
public:
    static constexpr const char* SensorServiceUuid = "4fafc201-1fb5-459e-1fcc-c5c9c331914b";

    static constexpr const char* TemperatureUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a0";
    static constexpr const char* HumidityUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a1";
    static constexpr const char* BearingUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a2";
    static constexpr const char* CompasTempUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a3";
    static constexpr const char* SpeedUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a4";
    static constexpr const char* WaterLevelUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a5";
    static constexpr const char* WaterPumpUsageUuid = "beb5483e-36e1-4688-a7f5-ea07361b26a6";

    explicit BluetoothSensorService(SensorCommandHandler* commandHandler);
    ~BluetoothSensorService() override;

    bool begin() override;
    const char* getServiceUUID() const override;
    const char* getServiceName() const override;
    void loop(unsigned long currentMillis) override;
    uint8_t getCharacteristicCount() const override;
    void* getBLEService() override;

private:
    SensorCommandHandler* _commandHandler;
    BLEService* _service;

    BLEFloatCharacteristic* _charTemperatureUsage;
    BLEFloatCharacteristic* _charHumidityUsage;
    BLEFloatCharacteristic* _charBearingUsage;
    BLEFloatCharacteristic* _charCompassTemperatureUsage;
    BLEIntCharacteristic* _charSpeedUsage;
    BLEIntCharacteristic* _charWaterLevelUsage;
    BLEByteCharacteristic* _charWaterPumpActive;

    unsigned long _lastUpdate;

    static constexpr unsigned long UpdateIntervalMs = 1000;

	void updateAll();
    void updateTemperature();
    void updateHumidity();
    void updateBearing();
    void updateCompassTemperature();
    void updateSpeed();
    void updateWaterLevel();
    void updateWaterPumpActive();
};

#endif