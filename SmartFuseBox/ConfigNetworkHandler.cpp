#include "ConfigNetworkHandler.h"
#include "ConfigManager.h"
#include "DateTimeManager.h"

ConfigNetworkHandler::ConfigNetworkHandler()
{
}

CommandResult ConfigNetworkHandler::handleRequest(const char* method,
	const char* command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	(void)params;
	(void)paramCount;

	return CommandResult::error(InvalidCommandParameters);
}

void ConfigNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	Config* config = ConfigManager::getConfigPtr();

	bool bluetoothEnabled = false;
	bool wifiEnabled = false;
	char ipAddress[MaxIpAddressLength] = "0.0.0.0";
	uint16_t port = DefaultWifiPort;
	uint8_t accessMode = 0;
	char ssid[MaxSSIDLength] = "";

	if (config)
	{
		bluetoothEnabled = config->bluetoothEnabled;
		wifiEnabled = config->wifiEnabled;
		port = config->wifiPort;
		accessMode = config->accessMode;
		snprintf(ssid, sizeof(ssid), config->apSSID);
		snprintf(ipAddress, sizeof(ipAddress), "%s", config->apIpAddress);
	}

	// Get runtime WiFi status if available

	// Enhanced JSON formatting with WiFi runtime details
	snprintf(buffer, size,
		"\"config\":{\"bluetooth\":%d,\"wifi\":%d,\"accessMode\":%d,\"ip\":\"%s\",\"ssid\":\"%s\",\"port\":%d}",
		bluetoothEnabled,
		wifiEnabled,
		accessMode,
		ipAddress,
		ssid,
		port);
}

void ConfigNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
