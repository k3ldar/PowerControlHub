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
#include "SmartFuseBoxConstants.h"


RelayCommandHandler::RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, RelayController* relayController)
    : _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink), _relayController(relayController)
{
}

RelayCommandHandler::~RelayCommandHandler()
{

}

const char* const* RelayCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { RelayTurnAllOff, RelayTurnAllOn, RelayRetrieveStates, 
        RelaySetState, RelayStatusGet };
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

    if (strncmp(command, RelayTurnAllOff, 2) == 0)
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
    else if (strncmp(command, RelayTurnAllOn, 2) == 0)
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
    else if (strncmp(command,  RelayRetrieveStates, 2) == 0)
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
    else if (strncmp(command, RelaySetState, 2) == 0)
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
    else if (strncmp(command, RelayStatusGet, 2) == 0)
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
	if (_relayController == nullptr)
    {
        return;
    }

    if (config == nullptr)
    {
        _relayController->setReservedSoundRelay(DefaultValue);
        return;
    }

    _relayController->setReservedSoundRelay(config->hornRelayIndex);

    // Defensive check: ensure it's either a valid relay or 0xFF (none)
    if (_relayController->getReservedSoundRelay() >= _relayController->getRelayCount() && _relayController->getReservedSoundRelay() != DefaultValue)
    {
        _relayController->setReservedSoundRelay(DefaultValue);

        if (_commandMgrComputer != nullptr)
        {
            _commandMgrComputer->sendDebug(F("Invalid hornRelayIndex corrected to 0xFF"), F("RELAY"));
        }
    }

    if (_commandMgrComputer != nullptr)
    {
        if (_relayController->getReservedSoundRelay() == DefaultValue)
        {
            _commandMgrComputer->sendDebug(F("Reserved sound relay: None (0xFF)"), F("RELAY"));
        }
        else
        {
            _commandMgrComputer->sendDebug(F("Reserved sound relay has been set"), F("RELAY"));
        }
    }
}

void RelayCommandHandler::broadcastRelayStatus(const char* cmd, const StringKeyValue* param)
{
    if (_commandMgrLink != nullptr)
    {
        sendAckOk(_commandMgrLink, cmd, param);
    }

    if (_commandMgrComputer != nullptr)
    {
        sendAckOk(_commandMgrComputer, cmd, param);
    }
}
