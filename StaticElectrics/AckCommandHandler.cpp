#include "AckCommandHandler.h"

const char AckCommand[] = "ACK";

AckCommandHandler::AckCommandHandler(SerialCommandManager* commandMgrComputer)
	: _commandMgrComputer(commandMgrComputer)
{
}

bool AckCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    _commandMgrComputer->sendDebug("Processing ACK: " + command + " (" + String(paramCount) + " params)", AckCommand);

    String cmd = command;
    cmd.trim();

    // Validate command
    if (cmd != AckCommand)
    {
        _commandMgrComputer->sendDebug("Unknown ACK command " + cmd, AckCommand);
        return false;
    }

    // the first param indicates what is being acknowledged (F0=ok for heartbeat ack, R2=ok for relay command ack, etc.)

    if (paramCount == 0)
    {
        _commandMgrComputer->sendDebug(F("No parameters in ACK command"), AckCommand);
        return false;
	}

    String key = params[0].key;
    key.trim();
	String val = params[0].value;
	val.trim();

    if (!val.equalsIgnoreCase(AckSuccess))
    {
        _commandMgrComputer->sendDebug("ACK indicates failure: key='" + key + "', val='" + val + "'", AckCommand);
		return false;
    }

	// only process known ACK keys if you need to take action
    
    sendAckOk(sender, cmd);
    return true;
}

const String* AckCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { AckCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}