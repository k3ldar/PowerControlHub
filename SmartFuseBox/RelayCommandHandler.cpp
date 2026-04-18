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
#include "RelayCommandHandler.h"
#include "ConfigManager.h"
#include "SmartFuseBoxConstants.h"
#include "SystemFunctions.h"


RelayCommandHandler::RelayCommandHandler(SerialCommandManager* commandMgrComputer, RelayController* relayController)
    : _commandMgrComputer(commandMgrComputer), _relayController(relayController),
      _config(nullptr)
{
}

RelayCommandHandler::~RelayCommandHandler()
{

}

const char* const* RelayCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { RelayTurnAllOff, RelayTurnAllOn, RelayRetrieveStates,
        RelaySetState, RelayStatusGet,
        RelayGetAllConfig, RelayRename, RelaySetButtonColor, RelaySetDefaultState,
        RelayLink, RelaySetActionType, RelaySetPin };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool RelayCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    if (!_relayController)
    {
        sendAckErr(sender, command, F("Relay controller not initialized"));
        return true;
	}

    if (SystemFunctions::commandMatches(command, RelayTurnAllOff))
    {
        if (paramCount == 0)
        {
			_relayController->turnAllRelaysOff();
			broadcastRelayStatus(command);
        }
        else
        {
			sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelayTurnAllOn))
    {
        if (paramCount == 0)
        {
			_relayController->turnAllRelaysOn();
            broadcastRelayStatus(command);
        }
        else
		{
            sendAckErr(sender, command, F("Invalid parameters"));
			return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelayRetrieveStates))
    {
        if (paramCount == 0)
        {
            // Retrieve states of all relays
            for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
            {
                CommandResult result = _relayController->getRelayStatus(i);

				uint8_t status = result.status;
                StringKeyValue param = makeParam(i, status);
                broadcastRelayStatus(command, &param);
            }

			broadcastRelayStatus(command);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
			return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelaySetState))
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = atoi(params[0].key);
            uint8_t state = atoi(params[0].value);

            if (relayIndex >= _relayController->getRelayCount())
            {
                sendAckErr(sender, command, F("Invalid relay index"));
                return true;
			}

			CommandResult result = _relayController->setRelayState(relayIndex, state > 0);
			RelayResult status = static_cast<RelayResult>(result.status);

            if (status == RelayResult::InvalidIndex)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
			}
            else if (status == RelayResult::Reserved)
            {
                sendAckErr(sender, command, F("Relay is reserved for sound system"), &params[0]);
                return true;
			}

			broadcastRelayStatus(command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, "Invalid parameters");
			return true;
        }
	}
    else if (SystemFunctions::commandMatches(command, RelayStatusGet))
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = atoi(params[0].key);

            if (relayIndex >= _relayController->getRelayCount())
            {
                sendAckErr(sender, command, "Invalid relay index");
                return true;
            }

            CommandResult result = _relayController->getRelayStatus(relayIndex);
			uint8_t status = result.status;
            StringKeyValue param = makeParam(relayIndex, status);
            broadcastRelayStatus(command, &param);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
			return true;
        }
	}
    else if (SystemFunctions::commandMatches(command, RelayGetAllConfig))
    {
        if (paramCount == 0)
        {
            sendAllRelayConfig(sender);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelayRename))
    {
        if (paramCount >= 1)
        {
            uint8_t idx = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));

            if (idx >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            if (SystemFunctions::calculateLength(params[0].value) == 0)
            {
                sendAckErr(sender, command, F("Missing name"), &params[0]);
                return true;
            }

            int pipeIdx = SystemFunctions::indexOf(params[0].value, '|', 0);
            char shortName[ConfigShortRelayNameLength] = "";
            char longName[ConfigLongRelayNameLength] = "";

            if (pipeIdx >= 0)
            {
                SystemFunctions::substr(shortName, sizeof(shortName), params[0].value, 0, pipeIdx);
                SystemFunctions::substr(longName, sizeof(longName), params[0].value, pipeIdx + 1);
            }
            else
            {
                strncpy(shortName, params[0].value, sizeof(shortName) - 1);
            }

            _relayController->renameRelay(idx, shortName, longName);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelaySetButtonColor))
    {
        if (paramCount >= 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t color = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relayIndex >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            if (color < DefaultValue)
                color += 2;

            if ((color < RelayImageButtonColorBlue || color > RelayImageButtonColorYellow) && color != DefaultValue)
            {
                sendAckErr(sender, command, F("Invalid color"), &params[0]);
                return true;
            }

            _relayController->setButtonColor(relayIndex, color);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelaySetDefaultState))
    {
        if (paramCount >= 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));

            if (relayIndex >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            _relayController->setRelayDefaultState(relayIndex, SystemFunctions::parseBooleanValue(params[0].value));
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelayLink))
    {
        if (paramCount >= 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t linkedRelay = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relayIndex >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            if (linkedRelay >= ConfigRelayCount && linkedRelay != DefaultValue)
            {
                sendAckErr(sender, command, F("Invalid linked relay (or 255 to unlink)"), &params[0]);
                return true;
            }

            if (linkedRelay == DefaultValue)
            {
                _relayController->unlinkRelay(relayIndex);
                sendAckOk(sender, command, &params[0]);
            }
            else
            {
                RelayResult linkResult = _relayController->linkRelays(relayIndex, linkedRelay);
                if (linkResult == RelayResult::Failed)
                {
                    sendAckErr(sender, command, F("No available link slots"));
                    return true;
                }
                sendAckOk(sender, command, &params[0]);
            }
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelaySetActionType))
    {
        if (paramCount >= 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t actionType = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relayIndex >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            if (actionType > static_cast<uint8_t>(RelayActionType::NightRelay))
            {
                sendAckErr(sender, command, F("Invalid action type"), &params[0]);
                return true;
            }

            _relayController->setRelayActionType(relayIndex, static_cast<RelayActionType>(actionType));
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else if (SystemFunctions::commandMatches(command, RelaySetPin))
    {
        if (paramCount >= 1)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[0].key, nullptr, 0));
            uint8_t pin = static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0));

            if (relayIndex >= ConfigRelayCount)
            {
                sendAckErr(sender, command, F("Invalid relay index"), &params[0]);
                return true;
            }

            if (pin == 0)
            {
                sendAckErr(sender, command, F("Invalid pin (use 255 to disable)"), &params[0]);
                return true;
            }

            _relayController->setRelayPin(relayIndex, pin);
            sendAckOk(sender, command, &params[0]);
        }
        else
        {
            sendAckErr(sender, command, F("Invalid parameters"));
            return true;
        }
    }
    else
    {
        sendAckErr(sender, command, F("Unknown relay command"));
        return true;
    }

	return true;
}

void RelayCommandHandler::setup()
{

}

void RelayCommandHandler::configUpdated(Config* config)
{
	_config = config;
}

void RelayCommandHandler::broadcastRelayStatus(const char* cmd, const StringKeyValue* param)
{
    if (_commandMgrComputer != nullptr)
    {
        sendAckOk(_commandMgrComputer, cmd, param);
    }
}

void RelayCommandHandler::sendAllRelayConfig(SerialCommandManager* sender)
{
    if (_config == nullptr || sender == nullptr)
        return;

    char buffer[64]{};

    // R6 — names
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        snprintf(buffer, sizeof(buffer), "%u=%s|%s", i,
            _config->relay.relays[i].shortName,
            _config->relay.relays[i].longName);
        sender->sendCommand(RelayRename, buffer);
    }

    // R7 — button colors
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        snprintf(buffer, sizeof(buffer), "%u=%u", i, _config->relay.relays[i].buttonImage);
        sender->sendCommand(RelaySetButtonColor, buffer);
    }

    // R8 — default states
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        snprintf(buffer, sizeof(buffer), "%u=%u", i, _config->relay.relays[i].defaultState ? 1 : 0);
        sender->sendCommand(RelaySetDefaultState, buffer);
    }

    // R9 — linked relays (only emit active pairs)
    for (uint8_t i = 0; i < ConfigMaxLinkedRelays; ++i)
    {
        if (_config->relay.linkedRelays[i][0] != DefaultValue)
        {
            snprintf(buffer, sizeof(buffer), "%u=%u",
                _config->relay.linkedRelays[i][0],
                _config->relay.linkedRelays[i][1]);
            sender->sendCommand(RelayLink, buffer);
        }
    }

    // R10 — action types
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        snprintf(buffer, sizeof(buffer), "%u=%u", i, static_cast<uint8_t>(_config->relay.relays[i].actionType));
        sender->sendCommand(RelaySetActionType, buffer);
    }

    // R11 — pins
    for (uint8_t i = 0; i < ConfigRelayCount; ++i)
    {
        snprintf(buffer, sizeof(buffer), "%u=%u", i, _config->relay.relays[i].pin);
        sender->sendCommand(RelaySetPin, buffer);
    }

    sendAckOk(sender, RelayGetAllConfig);
}
