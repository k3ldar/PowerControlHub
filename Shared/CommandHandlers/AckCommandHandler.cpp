#include "AckCommandHandler.h"

const char AckCommand[] = "ACK";

AckCommandHandler::AckCommandHandler(BroadcastManager* broadcastManager
#ifdef BOAT_CONTROL_PANEL
    , NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
#else
    )
	: SharedBaseCommandHandler(broadcastManager)
#endif // BOAT_CONTROL_PANEL
{
}

#ifdef BOAT_CONTROL_PANEL
bool AckCommandHandler::processHeartbeatAck(SerialCommandManager* sender, const String& key, const String& value)
{
    // Check for heartbeat acknowledgement (F0=ok)
    if (key != SystemHeartbeatCommand || !value.equalsIgnoreCase(AckSuccess))
        return false;

    if (_warningManager)
    {
        // Notify the warning manager to update heartbeat timestamp
        _warningManager->notifyHeartbeatAck();
	}

    // Notify the current page about the heartbeat acknowledgement
    notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::HeartbeatAck), nullptr);

    if (sender)
    {
        sender->sendDebug(F("Heartbeat ACK received"), AckCommand);
    }

    return true;
}
#endif

bool AckCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    sendDebugMessage("Processing ACK: " + command + " (" + String(paramCount) + " params)", AckCommand);
    
    String cmd = command;
    cmd.trim();

    // Validate command
    if (cmd != AckCommand)
    {
        sendDebugMessage("Unknown ACK command " + cmd, AckCommand);
        return false;
    }

    // the first param indicates what is being acknowledged (F0=ok for heartbeat ack, R2=ok for relay command ack, etc.)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in ACK command"), AckCommand);
        return false;
	}

    String key = params[0].key;
    key.trim();
	String val = params[0].value;
	val.trim();

    if (!val.equalsIgnoreCase(AckSuccess))
    {
        _broadcaster->sendDebug("ACK indicates failure: key='" + key + "', val='" + val + "'", AckCommand);
        return false;
    }

    // only process known ACK keys if you need to take action

#ifdef BOAT_CONTROL_PANEL
    if (key == SystemHeartbeatCommand && val.equalsIgnoreCase(AckSuccess))
    {
        // Heartbeat acknowledgement
        processHeartbeatAck(sender, key, val);
	}
    else if (key == RelayRetrieveStates && val.equalsIgnoreCase(AckSuccess))
    {
        // Relay state acknowledgement - handle both formats:
        // 1. ACK:R2=ok (just acknowledgement, no relay state - paramCount == 1)
        // 2. ACK:R2=ok:0=0 (acknowledgement with relay state - paramCount == 2)
        
        if (paramCount >= 2)
        {
            // Format: ACK:R2=ok:0=0 (with relay index and state)
            if (!isAllDigits(params[1].key) || !isAllDigits(params[1].value))
            {
                return true;
            }

            uint8_t relayIndex = params[1].key.toInt();
            bool isOn = parseBooleanValue(params[1].value);
            
            RelayStateUpdate update = { relayIndex, isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::RelayState), &update);
        }
    }
    else if (key == RelayStatusGet && val.equalsIgnoreCase(AckSuccess))
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = params[1].key.toInt();
            bool isOn = parseBooleanValue(params[1].value);

            RelayStateUpdate update = { relayIndex, isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::RelayState), &update);
        }
        else
        {
			sendDebugMessage("Invalid R4 ACK format: paramCount=" + String(paramCount), AckCommand);
        }
    }
    else if (key == SoundSignalActive && val.equalsIgnoreCase(AckSuccess))
    {
        if (paramCount == 1)
        {
            bool isOn = parseBooleanValue(params[1].value);

            BoolStateUpdate update = { isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::SoundSignal), &update);
        }
        else
        {
            sendDebugMessage("Invalid R4 ACK format: paramCount=" + String(paramCount), AckCommand);
        }
    }
#endif

    sendAckOk(sender, cmd);
    return true;
}

const String* AckCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { AckCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}