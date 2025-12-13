#include "RelayNetworkHandler.h"

RelayNetworkHandler::RelayNetworkHandler(RelayController* relayController)
	: _relayController(relayController)
{
}

CommandResult RelayNetworkHandler::handleRequest(const char* method,
	const char* command, StringKeyValue* params, uint8_t paramCount,
	char* responseBuffer, size_t bufferSize)
{
	(void)method;

	if (!_relayController)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Controller not initialized");
		return CommandResult::error(RelayControllerNotInitialised);
	}

	if (strcmp(command, RelayTurnAllOff) == 0)
	{
		_relayController->turnAllRelaysOff();
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (strcmp(command, RelayTurnAllOn) == 0)
	{
		_relayController->turnAllRelaysOn();
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (strcmp(command, RelayRetrieveStates) == 0)
	{
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (strcmp(command, RelaySetState) == 0)
	{
		if (paramCount == 1)
		{
			uint8_t relayIndex = atoi(params[0].key);
			uint8_t state = atoi(params[0].value);

			if (relayIndex >= _relayController->getRelayCount())
			{
				return CommandResult::error(InvalidCommandParameters);
			}

			CommandResult result = _relayController->setRelayState(relayIndex, state > 0);
			RelayResult status = static_cast<RelayResult>(result.status);

			if (status == RelayResult::InvalidIndex)
			{
				return CommandResult::error(InvalidCommandParameters);
			}
			else if (status == RelayResult::Reserved)
			{
				return CommandResult::error(InvalidCommandParameters);
			}

			formatStatusJson(responseBuffer, bufferSize);
			return CommandResult::ok();
		}
		else
		{
			return CommandResult::error(InvalidCommandParameters);
		}
	}
	else if (strcmp(command, RelayStatusGet) == 0)
	{
		if (paramCount == 1)
		{
			uint8_t relayIndex = atoi(params[0].key);

			if (relayIndex >= _relayController->getRelayCount())
			{
				return CommandResult::error(InvalidCommandParameters);
			}

			formatStatusJson(responseBuffer, bufferSize);
			return CommandResult::ok();
		}
		else
		{
			return CommandResult::error(InvalidCommandParameters);
		}
	}
	
	return CommandResult::error(InvalidCommandParameters);
}

void RelayNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	// Simple JSON formatting without library (to save memory)
	int written = snprintf(buffer, size, "\"relays\":[");
	
	for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
	{
		CommandResult result = _relayController->getRelayStatus(i);
		written += snprintf(buffer + written, size - written, 
						  "%d%s", result.status, 
						  (i < _relayController->getRelayCount() - 1) ? "," : "");
	}
	
	snprintf(buffer + written, size - written, "]");
}

void RelayNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
