#include "RelayCommandHandler.h"
#include "StaticElectricConstants.h"


RelayCommandHandler::RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, const uint8_t* relayPins, uint8_t totalRelays)
    : _relayStatus(nullptr), _relays(nullptr), _relayCount(totalRelays), _reservedSoundRelay(DefaultValue), 
    _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink)
{
    _relays = new uint8_t[_relayCount];
    memcpy(_relays, relayPins, sizeof(uint8_t) * _relayCount);

    _relayStatus = new bool[_relayCount];

    for (uint8_t i = 0; i < totalRelays; i++)
    {
        _relayStatus[i] = false;
    }
}

RelayCommandHandler::~RelayCommandHandler()
{
    delete[] _relays;
    delete[] _relayStatus;
}

const String* RelayCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { RelayTurnAllOff, RelayTurnAllOn, RelayRetrieveStates, 
        RelaySetState, RelayStatusGet };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool RelayCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    String cmd = command;
    cmd.trim();

    if (cmd == RelayTurnAllOff)
    {
        if (paramCount == 0)
        {
            // Turn all relays OFF
            for (int i = 0; i < _relayCount; i++)
            {
                setRelayStatus(i, false);
            }

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
            for (int i = 0; i < _relayCount; i++)
            {
                setRelayStatus(i, true);
			}
                    
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
            for (uint8_t i = 0; i < _relayCount; i++)
            {
                uint8_t status = getRelayStatus(i);
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

            if (relayIndex >= _relayCount)
            {
                sendAckErr(sender, cmd, F("Invalid relay index"));
                return true;
			}

            RelayResult status = setRelayStatus(relayIndex, state > 0);

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
            if (relayIndex >= _relayCount)
            {
                sendAckErr(sender, cmd, F("Invalid relay index"));
                return true;
            }

            uint8_t status = getRelayStatus(relayIndex);
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
    // Initialize all relay pins
    for (uint8_t i = 0; i < _relayCount; i++)
    {
        _relayStatus[i] = false;
        pinMode(_relays[i], OUTPUT);
        digitalWrite(_relays[i], HIGH);
    }

	Config* config = ConfigManager::getConfigPtr();

	if (config != nullptr)
    {
        _reservedSoundRelay = config->hornRelayIndex;
    }
}

RelayResult RelayCommandHandler::setRelayStatus(uint8_t relayIndex, bool isOn)
{
    if (relayIndex >= _relayCount)
    {
        return RelayResult::InvalidIndex;
    }

    if (relayIndex == _reservedSoundRelay)
    {
        return RelayResult::Reserved;
    }

    _relayStatus[relayIndex] = isOn;
    digitalWrite(_relays[relayIndex], isOn ? LOW : HIGH);

	return RelayResult::Success;
}

uint8_t RelayCommandHandler::getRelayStatus(uint8_t relayIndex) const
{
    if (relayIndex >= _relayCount)
    {
        return 0xFF;
    }

	return _relayStatus[relayIndex] ? 1 : 0;
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
