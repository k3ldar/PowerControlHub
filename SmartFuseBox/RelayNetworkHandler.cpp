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
	if (!buffer || size == 0)
		return;

	// Simple JSON formatting without library (to save memory)
	int written = snprintf(buffer, size, "\"relays\":[");

	if (written < 0 || written >= static_cast<int>(size))
	{
		buffer[size - 1] = '\0';
		return;
	}

	for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
	{
		CommandResult result = _relayController->getRelayStatus(i);
		int n = snprintf(buffer + written, size - written,
						"%d%s", result.status,
						(i < _relayController->getRelayCount() - 1) ? "," : "");

		if (n < 0 || written + n >= static_cast<int>(size))
			break;

		written += n;
	}

	snprintf(buffer + written, size - written, "]");
}

void RelayNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
