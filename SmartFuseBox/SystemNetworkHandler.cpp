#include "SystemNetworkHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "SystemFunctions.h"

SystemNetworkHandler::SystemNetworkHandler(WifiController* wifiController)
	: _wifiController(wifiController),
      _sdCardLogger(nullptr)
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

	if (_sdCardLogger)
	{
		sdPresent = _sdCardLogger->isSdCardPresent();
		logSize = _sdCardLogger->getCurrentLogFileSize();
	}

	snprintf(buffer, size,
		"\"system\":{\"mem\":%d,\"cpu\":%d,\"bluetooth\":%d,\"wifi\":%d,\"rssi\":%d,\"time\":\"%s\","
		"\"sd\":{\"present\":%d,\"log\":%lu}}",
		SystemFunctions::freeMemory(),
		SystemCpuMonitor::getCpuUsage(),
		bluetoothEnabled,
		wifiEnabled,
		rssi,
		dateTimeStr,
		sdPresent,
		(unsigned long)logSize);
}

void SystemNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
