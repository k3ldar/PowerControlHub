#pragma once

#include "Local.h"
#include "BaseSensor.h"
#include "MessageBus.h"

#include "WifiController.h"

#if defined(BLUETOOTH_SUPPORT)
#include "BluetoothController.h"
#endif

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

	WifiController* _wifiController;

#if defined(BLUETOOTH_SUPPORT)
	BluetoothController* _bluetoothController;
#endif

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
		WifiController* wifiController,

#if defined(BLUETOOTH_SUPPORT)
		BluetoothController* bluetoothController,
#endif
		WarningManager* warningManager)
		: _messageBus(messageBus),
		_wifiController(wifiController),

#if defined(BLUETOOTH_SUPPORT)
		_bluetoothController(bluetoothController),
#endif

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

	SensorIdList getSensorId() const override
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

	const char* getSensorName() const override
	{
		return "system";
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
			case 0: return { "Sys Free Memory", "free_memory",   nullptr,        "b",     false };
			case 1: return { "Sys CPU Usage",   "cpu_usage",     nullptr,        "%",     false };
			case 2: return { "Sys Bluetooth",   "bluetooth",     "connectivity", nullptr, true };
			case 3: return { "Sys WiFi",        "wifi",          "connectivity", nullptr, true };
			case 4: return { "Sys SD Log Size", "sd_log_size",   nullptr,        "MB",    false };
			case 5: return { "Sys Warnings",    "warning_count", nullptr,        nullptr, false };
			case 6: return { "Sys Uptime",      "uptime",        nullptr,        nullptr, false };
			default: return { nullptr, nullptr, nullptr, nullptr, false };
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
#if defined(BLUETOOTH_SUPPORT)
				snprintf(buffer, size, "%s",
					(_bluetoothController && _bluetoothController->isEnabled()) ? "ON" : "OFF");
#endif
				break;

			case 3:
				snprintf(buffer, size, "%s",
					(_wifiController && _wifiController->isEnabled()) ? "ON" : "OFF");
				break;

			case 4:
			{
				unsigned long bytes = (_sdCardLogger ? _sdCardLogger->getCurrentLogFileSize() : 0);
				double mb = static_cast<double>(bytes) / 1024.0 / 1024.0;
				snprintf(buffer, size, "%.1f", mb);
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
