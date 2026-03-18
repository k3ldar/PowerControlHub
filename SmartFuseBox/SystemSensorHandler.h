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
#include "BaseSensor.h"
#include "MessageBus.h"

#include "IWifiController.h"
#include "IBluetoothRadio.h"

#include "WarningManager.h"
#include "SystemFunctions.h"
#include "SystemCpuMonitor.h"
#include "SystemDefinitions.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardLogger.h"
#endif

constexpr unsigned long SystemSensorUpdateIntervalMs = 2500;

/**
 * @brief Sensor handler that exposes device diagnostics as MQTT sensor channels.
 *
 * Publishes free memory, CPU usage, Bluetooth/WiFi enabled state, SD log file
 * size and active warning count to Home Assistant via the standard sensor pipeline.
 * Values are read lazily from their respective managers in getMqttValue() and a
 * SystemMetricsUpdated event is published on a timed interval so that
 * MQTTSensorHandler republishes all sensor states.
 */
class SystemSensorHandler : public BaseSensor
{
private:
	MessageBus* _messageBus;

	IWifiController* _wifiController;
	IBluetoothRadio* _bluetoothRadio;

	WarningManager* _warningManager;

#if defined(SD_CARD_SUPPORT)
	SdCardLogger* _sdCardLogger;
#endif

	static uint8_t countActiveWarnings(uint32_t mask)
	{
		return static_cast<uint8_t>(__builtin_popcount(mask));
	}

protected:
	void initialize() override
	{}

	unsigned long update() override
	{
		if (_messageBus)
		{
			_messageBus->publish<SystemMetricsUpdated>();
		}

		return SystemSensorUpdateIntervalMs;
	}

public:
	SystemSensorHandler(MessageBus* messageBus, 
		IWifiController* wifiController,
		IBluetoothRadio* bluetoothController,
		WarningManager* warningManager)
		: BaseSensor("system"),
		_messageBus(messageBus),
		_wifiController(wifiController),
		_bluetoothRadio(bluetoothController),
		_warningManager(warningManager)

#if defined(SD_CARD_SUPPORT)
		, _sdCardLogger(nullptr)
#endif
	{}

#if defined(SD_CARD_SUPPORT)
	void setSdCardLogger(SdCardLogger* sdCardLogger)
	{
		_sdCardLogger = sdCardLogger;
	}
#endif

	void formatStatusJson(char* buffer, size_t size) override
	{
		snprintf(buffer, size, "\"freeMemory\":%u,\"cpuUsage\":%u",
			SystemFunctions::freeMemory(), SystemCpuMonitor::getCpuUsage());
	}

	SensorIdList getSensorIdType() const override
	{
		return SensorIdList::SystemSensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SystemFreeMemory;
	}

#if defined(MQTT_SUPPORT)
	uint8_t getMqttChannelCount() const override
	{
		return 7;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		switch (channelIndex)
		{
			case 0: return { "Sys Free Memory", "free_memory", "free_memory", nullptr, "b", false };
			case 1: return { "Sys CPU Usage", "cpu_usage", "cpu_usage", nullptr, "%", false };
			case 2: return { "Sys Bluetooth", "bluetooth", "bluetooth", "connectivity", nullptr, true };
			case 3: return { "Sys WiFi", "wifi", "wifi", "connectivity", nullptr, true };
			case 4: return { "Sys SD Log Size", "sd_log_size",   "sd_log_size", nullptr, "MB",    false };
			case 5: return { "Sys Warnings", "warning_count", "warning_count", nullptr, nullptr, false };
			case 6: return { "Sys Uptime", "uptime", "uptime", nullptr, nullptr, false };
			default: return { nullptr, nullptr, nullptr, nullptr, nullptr, false };
		}
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		switch (channelIndex)
		{
			case 0:
				snprintf(buffer, size, "%u", SystemFunctions::freeMemory());
				break;

			case 1:
				snprintf(buffer, size, "%u", SystemCpuMonitor::getCpuUsage());
				break;

			case 2:
				snprintf(buffer, size, "%s",
					(_bluetoothRadio && _bluetoothRadio->isEnabled()) ? "ON" : "OFF");
				break;

			case 3:
				snprintf(buffer, size, "%s",
					(_wifiController && _wifiController->isEnabled()) ? "ON" : "OFF");
				break;

					case 4:
					{
			#if defined(SD_CARD_SUPPORT)
						unsigned long bytes = (_sdCardLogger ? _sdCardLogger->getCurrentLogFileSize() : 0);
						double mb = static_cast<double>(bytes) / 1024.0 / 1024.0;
						snprintf(buffer, size, "%.1f", mb);
			#else
						snprintf(buffer, size, "0.0");
			#endif
						break;
					}

			case 5:
				snprintf(buffer, size, "%u",
					(_warningManager ? countActiveWarnings(_warningManager->getActiveWarningsMask()) : 0));
				break;

			case 6:
			{
				TimeParts timeParts = SystemFunctions::msToTimeParts(SystemFunctions::millis64());
				SystemFunctions::formatTimeParts(buffer, size, timeParts);
				break;
			}

			default:
				snprintf(buffer, size, "0");
				break;
		}
	}
#endif
};
