// 
// 
// 

#include "WarningNetworkHandler.h"


WarningNetworkHandler::WarningNetworkHandler(WarningManager* warningManager)
	: _warningManager(warningManager)
{
}

CommandResult WarningNetworkHandler::handleRequest(const char* method,
	const char* command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	(void)params;

	if (_warningManager == nullptr)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "warning controller not initialized");
		return CommandResult::error(WarningControllerNotInitialised);
	}

	// none of the warning commands should receive any parameters
	if (paramCount > 0)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Invalid parameters");
		return CommandResult::error(InvalidCommandParameters);
	}

	if (strcmp(command, WarningsList) == 0)
	{
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}

	return CommandResult::error(InvalidCommandParameters);
}

void WarningNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	snprintf(buffer, size, "\"warning\":{\"active\": \"0x%X\"}",
		static_cast<unsigned int>(_warningManager->getActiveWarningsMask()));
}

void WarningNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
