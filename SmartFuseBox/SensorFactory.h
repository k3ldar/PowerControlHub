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
#include "Config.h"
#include "BaseSensor.h"
#include "MessageBus.h"
#include "SensorCommandHandler.h"
#include "WarningManager.h"
#include "RelayController.h"
#include "IWifiController.h"
#include "IBluetoothRadio.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardLogger.h"
#endif

#include "WaterSensorHandler.h"
#include "Dht11SensorHandler.h"
#include "LightSensorHandler.h"
#include "SystemSensorHandler.h"

/**
 * @brief Dependencies injected by SmartFuseBoxApp into SensorFactory::create().
 *
 * Grouping them into a struct keeps the create() signature manageable and means
 * adding a new shared dependency does not require touching every call-site.
 */
struct SensorFactoryContext
{
    MessageBus*           messageBus;
    BroadcastManager*     broadcastManager;
    SensorCommandHandler* sensorCommandHandler;
    WarningManager*       warningManager;
    RelayController*      relayController;
    IWifiController*      wifiController;
    IBluetoothRadio*      bluetoothRadio;

#if defined(SD_CARD_SUPPORT)
    SdCardLogger*         sdCardLogger;
#endif
};

/**
 * @brief Boot-time factory that converts SensorsConfig entries into live BaseSensor instances.
 *
 * Called once during setup(), after ConfigManager::load() has populated SensorsConfig.
 * All allocations are permanent for the lifetime of the device — no sensor is ever
 * destroyed or recreated at runtime.  A reboot is required for config changes to
 * take effect, which is intentional.
 *
 * Usage (inside SmartFuseBoxApp::setup):
 * @code
 *   SensorFactoryContext ctx{ &_messageBus, ... };
 *   uint8_t factoryCount = 0;
 *   BaseSensor** factorySensors = SensorFactory::create(config->sensors, ctx, factoryCount);
 * @endcode
 */
class SensorFactory
{
public:
    /**
     * @brief Allocate and initialise sensors described by @p sensorsConfig.
     *
     * Only entries with `enabled == true` are instantiated.  Entries whose
     * `sensorType` is not recognised are silently skipped — a warning is raised
     * if a WarningManager is available in @p ctx.
     *
     * @param sensorsConfig  Persistent config read from EEPROM.
     * @param ctx            Shared application-level dependencies.
     * @param[out] outCount  Number of sensors successfully created.
     * @return Heap-allocated array of BaseSensor* (may be nullptr if count == 0).
     */
    static BaseSensor** create(const SensorsConfig& sensorsConfig,
                               const SensorFactoryContext& ctx,
                               uint8_t& outCount)
    {
        outCount = 0;

        // First pass: count how many enabled config entries we can actually build,
        // excluding any SystemSensor entries (the system sensor is always added automatically)
        uint8_t capacity = 1; // +1 reserved for the automatic SystemSensor
        for (uint8_t i = 0; i < sensorsConfig.count && i < ConfigMaxSensors; i++)
        {
            if (sensorsConfig.sensors[i].enabled &&
                sensorsConfig.sensors[i].sensorType != SensorIdList::SystemSensor)
                capacity++;
        }

        BaseSensor** sensors = new BaseSensor*[capacity];

        for (uint8_t i = 0; i < sensorsConfig.count && i < ConfigMaxSensors; i++)
        {
            const SensorEntry& entry = sensorsConfig.sensors[i];

            if (!entry.enabled || entry.sensorType == SensorIdList::SystemSensor)
                continue;

            BaseSensor* sensor = createOne(entry, ctx);

            if (sensor != nullptr)
            {
                sensors[outCount++] = sensor;
            }
            else if (ctx.warningManager != nullptr)
            {
                ctx.warningManager->raiseWarning(WarningType::SensorFailure);
            }
        }

        // Always append the system sensor as the last entry
        sensors[outCount++] = createSystemSensor(ctx);

        return sensors;
    }

private:
    SensorFactory() = delete;

    static SystemSensorHandler* createSystemSensor(const SensorFactoryContext& ctx)
    {
        SystemSensorHandler* s = new SystemSensorHandler(
            ctx.messageBus,
            ctx.wifiController,
            ctx.bluetoothRadio,
            ctx.warningManager);

#if defined(SD_CARD_SUPPORT)
        if (ctx.sdCardLogger != nullptr)
            s->setSdCardLogger(ctx.sdCardLogger);
#endif

        return s;
    }

    static BaseSensor* createOne(const SensorEntry& entry, const SensorFactoryContext& ctx)
    {
        switch (entry.sensorType)
        {
            case SensorIdList::WaterSensor:
            {
                // pins[0] = sensor pin, pins[1] = active/power pin
                uint8_t sensorPin = entry.pins[0];
                uint8_t activePin = entry.pins[1];

                return new WaterSensorHandler(
                    ctx.messageBus,
                    ctx.broadcastManager,
                    ctx.sensorCommandHandler,
                    sensorPin,
                    activePin,
                    entry.name);
            }

            case SensorIdList::Dht11Sensor:
            {
                // pins[0] = data pin
                uint8_t sensorPin = entry.pins[0];
				float humidityOffset = static_cast<float>(entry.options2[0]) / 10.0f; // e.g. -5..+5% in steps of 0.1
                float temperatureOffset = static_cast<float>(entry.options2[1]) / 10.0f;

                return new Dht11SensorHandler(
                    ctx.messageBus,
                    ctx.broadcastManager,
                    ctx.sensorCommandHandler,
                    ctx.warningManager,
                    sensorPin,
                    humidityOffset,
                    temperatureOffset,
                    entry.name);
            }

            case SensorIdList::LightSensor:
            {
                // pins[0] = sensor pin, options1[0] != 0 means digital mode
                uint8_t sensorPin = entry.pins[0];
                bool    isDigital = (entry.options1[0] != 0);

                return new LightSensorHandler(
                    ctx.messageBus,
                    ctx.broadcastManager,
                    ctx.sensorCommandHandler,
                    ctx.relayController,
                    sensorPin,
                    isDigital,
                    entry.name);
            }

            default:
                return nullptr;
        }
    }
};
