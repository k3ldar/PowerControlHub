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
#include "SystemFunctions.h"

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

	if (SystemFunctions::commandMatches(command, RelayTurnAllOff))
	{
		_relayController->turnAllRelaysOff();
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (SystemFunctions::commandMatches(command, RelayTurnAllOn))
	{
		_relayController->turnAllRelaysOn();
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (SystemFunctions::commandMatches(command, RelayRetrieveStates))
	{
		formatStatusJson(responseBuffer, bufferSize);
		return CommandResult::ok();
	}
	else if (SystemFunctions::commandMatches(command, RelaySetState))
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


			if (!result.success)
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
	else if (SystemFunctions::commandMatches(command, RelayStatusGet))
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

	Config* config = ConfigManager::getConfigPtr();

	int written = snprintf(buffer, size, "\"relays\":[");

	if (written < 0 || written >= static_cast<int>(size))
	{
		buffer[size - 1] = '\0';
		return;
	}

	for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
	{
		CommandResult result = _relayController->getRelayStatus(i);
		int n;

		if (config != nullptr && i < ConfigRelayCount)
		{
			const RelayEntry& relay = config->relay.relays[i];
			n = snprintf(buffer + written, size - written,
				"%s{\"sn\":\"%s\",\"pn\":%u,\"bt\":%u,\"df\":%u,\"at\":%u,\"st\":%u}",
				(i > 0) ? "," : "",
				relay.shortName,
				relay.pin,
				relay.buttonImage,
				relay.defaultState ? 1u : 0u,
				static_cast<uint8_t>(relay.actionType),
				result.status);
		}
		else
		{
			n = snprintf(buffer + written, size - written,
				"%s%u",
				(i > 0) ? "," : "",
				result.status);
		}

		if (n < 0 || written + n >= static_cast<int>(size))
			break;

		written += n;
	}

	int n = snprintf(buffer + written, size - written, "]");
	if (n < 0 || written + n >= static_cast<int>(size))
	{
		buffer[size - 1] = '\0';
		return;
	}
	written += n;

	if (config != nullptr)
	{
		n = snprintf(buffer + written, size - written,
			",\"hm\":[%u,%u,%u,%u]",
			config->relay.homePageMapping[0],
			config->relay.homePageMapping[1],
			config->relay.homePageMapping[2],
			config->relay.homePageMapping[3]);

		if (n < 0 || written + n >= static_cast<int>(size))
		{
			buffer[size - 1] = '\0';
			return;
		}
		written += n;

		n = snprintf(buffer + written, size - written, ",\"lk\":[");
		if (n < 0 || written + n >= static_cast<int>(size))
		{
			buffer[size - 1] = '\0';
			return;
		}
		written += n;

		for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
		{
			n = snprintf(buffer + written, size - written,
				"%s[%u,%u]",
				(i > 0) ? "," : "",
				config->relay.linkedRelays[i][0],
				config->relay.linkedRelays[i][1]);

			if (n < 0 || written + n >= static_cast<int>(size))
			{
				buffer[size - 1] = '\0';
				return;
			}
			written += n;
		}

		n = snprintf(buffer + written, size - written, "]");
		if (n < 0 || written + n >= static_cast<int>(size))
		{
			buffer[size - 1] = '\0';
		}
	}
}

void RelayNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	Config* config = ConfigManager::getConfigPtr();
	uint8_t relayCount = _relayController->getRelayCount();

	client->print("\"relays\":[");

	for (uint8_t i = 0; i < relayCount; i++)
	{
		CommandResult result = _relayController->getRelayStatus(i);

		if (i > 0)
			client->print(",");

		if (config != nullptr && i < ConfigRelayCount)
		{
			const RelayEntry& relay = config->relay.relays[i];
			client->print("{\"shortName\":\"");
			client->print(relay.shortName);
			client->print("\",\"longName\":\"");
			client->print(relay.longName);
			client->print("\",\"pin\":");
			client->print(relay.pin);
			client->print(",\"img\":");
			client->print(relay.buttonImage);
			client->print(",\"defaultState\":");
			client->print(relay.defaultState ? 1 : 0);
			client->print(",\"actionType\":");
			client->print(static_cast<uint8_t>(relay.actionType));
			client->print(",\"state\":");
			client->print(result.status);
			client->print("}");
		}
		else
		{
			client->print(result.status);
		}
	}

	client->print("]");

	if (config != nullptr)
	{
		client->print(",\"homeMap\":[");
		for (uint8_t i = 0; i < ConfigHomeButtons; i++)
		{
			if (i > 0)
				client->print(",");
			client->print(config->relay.homePageMapping[i]);
		}
		client->print("]");

		client->print(",\"linked\":[");
		for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
		{
			if (i > 0)
				client->print(",");
			client->print("[");
			client->print(config->relay.linkedRelays[i][0]);
			client->print(",");
			client->print(config->relay.linkedRelays[i][1]);
			client->print("]");
		}
		client->print("]");
	}
}
