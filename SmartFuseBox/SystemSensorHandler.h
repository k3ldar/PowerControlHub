#pragma once

#include "BaseSensor.h"
#include "MessageBus.h"
#include "WifiController.h"
#include "BluetoothController.h"
#include "WarningManager.h"
#include "SystemFunctions.h"
#include "SystemCpuMonitor.h"
#include "SystemDefinitions.h"
#include "SdCardLogger.h"

constexpr unsigned long SystemSensorUpdateIntervalMs = 5000;

/**
 * @brief Sensor handler that exposes device diagnostics as MQTT sensor channels.
 *
 * Publishes free memory, CPU usage, Bluetooth/WiFi enabled state, SD log file
 * size and active warning count to Home Assistant via the standard sensor pipeline.
 * Values are read lazily from their respective managers in getMqttValue() and a
 * SystemMetricsUpdated event is published on a 30-second interval so that
 * MQTTSensorHandler marks all channels dirty and republishes.
 */
class SystemSensorHandler : public BaseSensor
{
private:
	MessageBus* _messageBus;
	WifiController* _wifiController;
	BluetoothController* _bluetoothController;
	WarningManager* _warningManager;
	SdCardLogger* _sdCardLogger;
	unsigned long _nextPublishTime;

	static uint8_t countActiveWarnings(uint32_t mask)
	{
		return static_cast<uint8_t>(__builtin_popcount(mask));
	}

protected:
	void initialize() override
	{
	}

	unsigned long update() override
	{
		return SystemSensorUpdateIntervalMs;
	}

public:
	SystemSensorHandler(MessageBus* messageBus, WifiController* wifiController,
		BluetoothController* bluetoothController, WarningManager* warningManager)
		: _messageBus(messageBus),
		  _wifiController(wifiController),
		  _bluetoothController(bluetoothController),
		  _warningManager(warningManager),
		  _sdCardLogger(nullptr),
		  _nextPublishTime(0)
	{
	}

	void setSdCardLogger(SdCardLogger* sdCardLogger)
	{
		_sdCardLogger = sdCardLogger;
	}

	/**
	 * @brief Call from loop() to publish SystemMetricsUpdated on the configured interval.
	 * @param now Current time from millis().
	 */
	void loopUpdate(unsigned long now)
	{
		if (now >= _nextPublishTime)
		{
			if (_messageBus)
			{
				_messageBus->publish<SystemMetricsUpdated>();
			}

			_nextPublishTime = now + SystemSensorUpdateIntervalMs;
		}
	}

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

	uint8_t getMqttChannelCount() const override
	{
		return 6;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		switch (channelIndex)
		{
		case 0: return { "Free Memory", "free_memory",   nullptr,        "B",   false };
		case 1: return { "CPU Usage",   "cpu_usage",     nullptr,        "%",   false };
		case 2: return { "Bluetooth",   "bluetooth",     "connectivity", nullptr, true };
		case 3: return { "WiFi",        "wifi",          "connectivity", nullptr, true };
		case 4: return { "SD Log Size", "sd_log_size",   nullptr,        "B",   false };
		case 5: return { "Warnings",    "warning_count", nullptr,        nullptr, false };
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
			snprintf(buffer, size, "%s",
				(_bluetoothController && _bluetoothController->isEnabled()) ? "ON" : "OFF");
			break;
		case 3:
			snprintf(buffer, size, "%s",
				(_wifiController && _wifiController->isEnabled()) ? "ON" : "OFF");
			break;
		case 4:
			snprintf(buffer, size, "%lu",
				(unsigned long)(_sdCardLogger ? _sdCardLogger->getCurrentLogFileSize() : 0));
			break;
		case 5:
			snprintf(buffer, size, "%u",
				(_warningManager ? countActiveWarnings(_warningManager->getActiveWarningsMask()) : 0));
			break;
		default:
			snprintf(buffer, size, "0");
			break;
		}
	}
};
