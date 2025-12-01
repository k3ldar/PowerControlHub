#include "RelayCommandHandler.h"
#include "SmartFuseBoxConstants.h"


RelayCommandHandler::RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, RelayController* relayController)
    : _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink), _relayController(relayController)
{
}

RelayCommandHandler::~RelayCommandHandler()
{

}

const String* RelayCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { RelayTurnAllOff, RelayTurnAllOn, RelayRetrieveStates, 
        RelaySetState, RelayStatusGet };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool RelayCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
{
    if (!_relayController)
    {
        sendAckErr(sender, command, F("Relay controller not initialized"));
        return true;
	}

    String cmd = command;
    cmd.trim();

    if (cmd == RelayTurnAllOff)
    {
        if (paramCount == 0)
        {
			_relayController->turnAllRelaysOff();
			broadcastRelayStatus(cmd);
        }
        else
        {
			sendAckErr(sender, cmd, F("Invalid parameters"));
            return true;
        }
    }
    else if (cmd == RelayTurnAllOn)
    {
        if (paramCount == 0)
        {
			_relayController->turnAllRelaysOn();
            broadcastRelayStatus(cmd);
        }
        else
		{
            sendAckErr(sender, cmd, F("Invalid parameters"));
			return true;
        }
    }
    else if (cmd == RelayRetrieveStates)
    {
        if (paramCount == 0)
        {
            // Retrieve states of all relays
            for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
            {
                CommandResult result = _relayController->getRelayStatus(i);

				uint8_t status = result.status;
                StringKeyValue param = { String(i), String(status) };
                broadcastRelayStatus(cmd, &param);
            }

			broadcastRelayStatus(cmd);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid parameters"));
			return true;
        }
    }
    else if (cmd == RelaySetState)
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = params[0].key.toInt();
            uint8_t state = params[0].value.toInt();

            if (relayIndex >= _relayController->getRelayCount())
            {
                sendAckErr(sender, cmd, F("Invalid relay index"));
                return true;
			}

			CommandResult result = _relayController->setRelayState(relayIndex, state > 0);
			RelayResult status = static_cast<RelayResult>(result.status);

            if (status == RelayResult::InvalidIndex)
            {
                sendAckErr(sender, cmd, F("Invalid relay index"), &params[0]);
                return true;
			}
            else if (status == RelayResult::Reserved)
            {
                sendAckErr(sender, cmd, F("Relay is reserved for sound system"), &params[0]);
                return true;
			}

			broadcastRelayStatus(cmd, &params[0]);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid parameters"));
			return true;
        }
	}
    else if (cmd == RelayStatusGet)
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = params[0].key.toInt();

            if (relayIndex >= _relayController->getRelayCount())
            {
                sendAckErr(sender, cmd, F("Invalid relay index"));
                return true;
            }

            CommandResult result = _relayController->getRelayStatus(relayIndex);
			uint8_t status = result.status;
            StringKeyValue param = { String(relayIndex), String(status) };
            broadcastRelayStatus(cmd, &param);
        }
        else
        {
            sendAckErr(sender, cmd, F("Invalid parameters"));
			return true;
        }
	}
    else
    {
        sendAckErr(sender, cmd, F("Unknown relay command"));
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
            _commandMgrComputer->sendDebug(
                F("Invalid hornRelayIndex corrected to 0xFF"),
                F("RELAY")
            );
        }
    }

    if (_commandMgrComputer != nullptr)
    {
        String msg = F("Reserved sound relay: ");
        if (_relayController->getReservedSoundRelay() == DefaultValue)
        {
            msg += F("None (0xFF)");
        }
        else
        {
            msg += String(_relayController->getReservedSoundRelay());
        }

        _commandMgrComputer->sendDebug(msg, F("RELAY"));
    }
}

void RelayCommandHandler::broadcastRelayStatus(const String& cmd, const StringKeyValue* param)
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
