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
#include "SystemNetworkHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "SystemFunctions.h"

#if defined(SD_CARD_SUPPORT)
#include "MicroSdDriver.h"
#endif

constexpr uint8_t UptimeBufferLength = 15;

SystemNetworkHandler::SystemNetworkHandler(WifiController* wifiController)
	: _wifiController(wifiController)
#if defined(SD_CARD_SUPPORT)
	, _sdCardLogger(nullptr)
#endif
{
}

CommandResult SystemNetworkHandler::handleRequest(const char* method,
	const char* command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	(void)params;
	(void)paramCount;

	if (strcmp(command, SystemHeartbeatCommand) == 0)
	{
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else
	{
		return CommandResult::error(InvalidCommandParameters);
	}
}

void SystemNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	Config* config = ConfigManager::getConfigPtr();

	bool bluetoothEnabled = false;
	bool wifiEnabled = false;
	int rssi = 0;

	if (config)
	{
		bluetoothEnabled = config->bluetoothEnabled;
		wifiEnabled = config->wifiEnabled;
	}

	if (_wifiController && wifiEnabled && _wifiController->isEnabled())
	{
		rssi = _wifiController->getServer()->getSignalStrength();
	}

	char dateTimeStr[DateTimeBufferLength];
	DateTimeManager::formatDateTime(dateTimeStr, sizeof(dateTimeStr));

	bool sdPresent = false;
	uint32_t logSize = 0;

#if defined(SD_CARD_SUPPORT)
	if (_sdCardLogger)
	{
		MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
		sdPresent = sdDriver.isCardPresent();
		logSize = _sdCardLogger->getCurrentLogFileSize();
	}
#endif

	char uptime[UptimeBufferLength];
	TimeParts timeParts = SystemFunctions::msToTimeParts(SystemFunctions::millis64());
	SystemFunctions::formatTimeParts(uptime, UptimeBufferLength, timeParts);


	snprintf(buffer, size,
		"\"system\":{\"mem\":%d,\"cpu\":%d,\"bluetooth\":%d,\"wifi\":%d,\"rssi\":%d,\"time\":\"%s\","
		"\"sd\":{\"present\":%d,\"log\":%lu},\"Uptime\":\"%s\"}",
		SystemFunctions::freeMemory(),
		SystemCpuMonitor::getCpuUsage(),
		bluetoothEnabled,
		wifiEnabled,
		rssi,
		dateTimeStr,
		sdPresent,
		(unsigned long)logSize,
		uptime);
}

void SystemNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
