#include "AckCommandHandler.h"

const char AckCommand[] = "ACK";

#if defined(BOAT_CONTROL_PANEL)
AckCommandHandler::AckCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
#elif defined(FUSE_BOX_CONTROLLER)
AckCommandHandler::AckCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcastManager, warningManager)
#endif
{
}

#ifdef BOAT_CONTROL_PANEL
bool AckCommandHandler::processHeartbeatAck(SerialCommandManager* sender, const String& key, const String& value)
{
    // Check for heartbeat acknowledgement (F0=ok)
    if (key != SystemHeartbeatCommand || !value.equalsIgnoreCase(AckSuccess))
        return false;

    if (getWarningManager())
    {
        // Notify the warning manager to update heartbeat timestamp
        getWarningManager()->notifyHeartbeatAck();
	}

    // Notify the current page about the heartbeat acknowledgement
    notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::HeartbeatAck), nullptr);

    if (sender)
    {
        sender->sendDebug(F("Heartbeat ACK received"), AckCommand);
    }

    return true;
}

bool AckCommandHandler::processWarningsListAck(SerialCommandManager* sender, const String& key, const String& value, const StringKeyValue params[], uint8_t paramCount)
{
    // Check for warnings list acknowledgement (W1=ok)
    if (key != WarningsList || !value.equalsIgnoreCase(AckSuccess))
        return false;

    WarningManager* warningManager = getWarningManager();
    if (!warningManager)
    {
        sendDebugMessage(F("Warning manager not available"), AckCommand);
        return false;
    }

    // Format: ACK:W1=ok:v=0x06 (paramCount == 2)
    // The warning bitmask is in params[1].value
    if (paramCount >= 2)
    {
        String warningKey = params[1].key;
        warningKey.trim();
        String warningValue = params[1].value;
        warningValue.trim();

        if (warningKey == ValueParamName)
        {
            // Parse the hexadecimal bitmask
            uint32_t remoteWarningMask = 0;
            
            if (warningValue.startsWith(F("0x")) || warningValue.startsWith(F("0X")))
            {
                // Parse hexadecimal (skip the "0x" prefix)
                remoteWarningMask = strtoul(warningValue.c_str() + 2, nullptr, 16);
            }
            else if (isAllDigits(warningValue))
            {
                // Parse decimal
                remoteWarningMask = warningValue.toInt();
            }
            else
            {
                sendDebugMessage("Invalid warning mask format: " + warningValue, AckCommand);
                return false;
            }

            // UPDATE (replace) remote warnings - this allows remote warnings to be cleared
            warningManager->updateRemoteWarnings(remoteWarningMask);

            // Notify the warning page to update display
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Warning), nullptr);

            if (sender)
            {
                sender->sendDebug("Remote warnings updated: 0x" + String(remoteWarningMask, HEX), AckCommand);
            }

            return true;
        }
    }
    else
    {
        // No warning data provided - clear remote warnings
        warningManager->updateRemoteWarnings(0);
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Warning), nullptr);
        
        if (sender)
        {
            sender->sendDebug(F("W1 ACK received (no warnings)"), AckCommand);
        }
        return true;
    }

    return false;
}
#endif

bool AckCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)sender;
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

    // Ignore redundant ACK:ACK=ok messages
    if (key.equalsIgnoreCase(AckCommand))
    {
        return true; // Silently ignore, consider it handled
    }

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
    else if (key == WarningsList && val.equalsIgnoreCase(AckSuccess))
    {
        // Warnings list acknowledgement - merge remote warnings
        processWarningsListAck(sender, key, val, params, paramCount);
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
        if (paramCount >= 2)
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
        if (paramCount >= 2)
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
	else if (key == SystemCpuUsage && val.equalsIgnoreCase(AckSuccess))
    {
        if (paramCount >= 2)
        {
            uint8_t cpuUsage = params[1].value.toInt();
            UInt8Update update = { cpuUsage };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CpuUsage), &update);
        }
        else
        {
            sendDebugMessage("Invalid F3 ACK format: paramCount=" + String(paramCount), AckCommand);
        }
    }
    else if (key == SystemFreeMemory && val.equalsIgnoreCase(AckSuccess))
    {
        if (paramCount >= 2)
        {
            uint16_t freeMemory = params[1].value.toInt();
            UInt16Update update = { freeMemory };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::MemoryUsage), &update);
        }
        else
        {
            sendDebugMessage("Invalid F2 ACK format: paramCount=" + String(paramCount), AckCommand);
        }
	}
#endif

    return true;
}

const String* AckCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { AckCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}