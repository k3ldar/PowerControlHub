// 
// 
// 

#include "WarningNetworkHandler.h"


WarningNetworkHandler::WarningNetworkHandler(WarningManager* warningManager)
	: _warningManager(warningManager)
{
}

CommandResult WarningNetworkHandler::handleRequest(const String& method,
	const String& command,
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

	String cmd = command;
	cmd.trim();

	// none of the warning commands should receive any parameters
	if (paramCount > 0)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Invalid parameters");
		return CommandResult::error(InvalidCommandParameters);
	}

	if (cmd == WarningsList)
	{
		formatWarningStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}

	formatJsonResponse(responseBuffer, bufferSize, false, "Invalid command");
	return CommandResult::error(InvalidCommandParameters);
}

void WarningNetworkHandler::formatWarningStatusJson(char* buffer, size_t size)
{
	snprintf(buffer, size, "{\"warning\":{\"active\": \"0x%X\"}}",
		_warningManager->getActiveWarningsMask());
}

void WarningNetworkHandler::formatJsonResponse(char* buffer, size_t size, bool success, const char* message)
{
	if (message)
	{
		snprintf(buffer, size, "{\"success\":%s,\"message\":\"%s\"}",
			success ? "true" : "false", message);
	}
	else
	{
		snprintf(buffer, size, "{\"success\":%s}", success ? "true" : "false");
	}
}