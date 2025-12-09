#include "SystemNetworkHandler.h"
#include "SystemCpuMonitor.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"
#include "SharedFunctions.h"

SystemNetworkHandler::SystemNetworkHandler(WifiController* wifiController)
	: _wifiController(wifiController)
{
}

CommandResult SystemNetworkHandler::handleRequest(const String& method,
	const String& command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	(void)params;
	(void)paramCount;

	String cmd = command;
	cmd.trim();

	if (cmd == SystemHeartbeatCommand)
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
	String ipAddress = F("0.0.0.0");
	String ssid = "";
	int rssi = 0;

	if (config)
	{
		bluetoothEnabled = config->bluetoothEnabled;
		wifiEnabled = config->wifiEnabled;
	}

	// Get runtime WiFi status if available
	if (_wifiController && wifiEnabled && _wifiController->isEnabled())
	{
		ipAddress = _wifiController->getServer()->getIpAddress();
		ssid = _wifiController->getServer()->getSSID();
		rssi = _wifiController->getServer()->getSignalStrength();
	}

	// Enhanced JSON formatting with WiFi runtime details
	snprintf(buffer, size,
		"\"system\":{\"mem\":%d,\"cpu\":%d,\"bluetooth\":%d,\"wifi\":%d,\"ip\":\"%s\",\"ssid\":\"%s\",\"rssi\":%d,\"time\":\"%s\"}",
		SharedFunctions::freeMemory(),
		SystemCpuMonitor::getCpuUsage(),
		bluetoothEnabled,
		wifiEnabled,
		ipAddress.c_str(),
		ssid.c_str(),
		rssi,
		DateTimeManager::formatDateTime().c_str());
}

void SystemNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
