#include "AckCommandHandler.h"
#include "SystemFunctions.h"


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

#if defined(BOAT_CONTROL_PANEL)
bool AckCommandHandler::processHeartbeatAck(SerialCommandManager* sender, const char* key, const char* value)
{
    // Check for heartbeat acknowledgement (F0=ok)
    if (strcmp(key, SystemHeartbeatCommand) != 0 || strcmp(value, AckSuccess) != 0)
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
        sender->sendDebug("Heartbeat ACK received", AckCommand);
    }

    return true;
}

bool AckCommandHandler::processWarningsListAck(SerialCommandManager* sender, const char* key, const char* value, const StringKeyValue params[], uint8_t paramCount)
{
    // Check for warnings list acknowledgement (W1=ok)
    if (strcmp(key, SystemHeartbeatCommand) != 0 || strcmp(value, AckSuccess) != 0)
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
        if (SystemFunctions::startsWith(params[1].key, ValueParamName))
        {
            // Parse the hexadecimal bitmask
            uint32_t remoteWarningMask = 0;
            
            if (SystemFunctions::startsWith(params[1].value, F("0x")) || SystemFunctions::startsWith(params[1].value, F("0X")))
            {
                // Parse hexadecimal (skip the "0x" prefix)
                remoteWarningMask = strtoul(params[1].value, nullptr, 16);
            }
            else if (SystemFunctions::isAllDigits(params[1].value))
            {
                // Parse decimal
                remoteWarningMask = static_cast<uint8_t>(strtoul(params[1].value, nullptr, 0));
            }
            else
            {
                sendDebugMessage(F("Invalid warning mask format"), AckCommand);
                return false;
            }

            // UPDATE (replace) remote warnings - this allows remote warnings to be cleared
            warningManager->updateRemoteWarnings(remoteWarningMask);

            // Notify the warning page to update display
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Warning), nullptr);

            if (sender)
            {
                sender->sendDebug("Remote warnings updated", AckCommand);
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
            sender->sendDebug("W1 ACK received (no warnings)", AckCommand);
        }
        return true;
    }

    return false;
}
#endif

bool AckCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)sender;

    // Validate command
    if (strncmp(command, AckCommand, 3) != 0)
    {
        char debugMsg[64];
        snprintf(debugMsg, sizeof(debugMsg), "Unknown ACK command %s", command);
        sendDebugMessage(debugMsg, AckCommand);
        return false;
    }

    // the first param indicates what is being acknowledged (F0=ok for heartbeat ack, R2=ok for relay command ack, etc.)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in ACK command"), AckCommand);
        return false;
	}

    // Ignore redundant ACK:ACK=ok messages
    if (strncmp(params[0].key, AckCommand, 3) == 0)
    {
        return true; // Silently ignore, consider it handled
    }

    if (strcmp(params[0].value, AckSuccess) != 0)
    {
        char debugMsg[100];
        snprintf(debugMsg, sizeof(debugMsg), "ACK indicates failure: key='%s', val='%s'", params[0].key, params[0].value);
        _broadcaster->sendDebug(debugMsg, AckCommand);
        return false;
    }

    // only process known ACK keys if you need to take action

#if defined(BOAT_CONTROL_PANEL)
    if (strcmp(params[0].key, SystemHeartbeatCommand) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        // Heartbeat acknowledgement
        processHeartbeatAck(sender, params[0].key, params[0].value);
	}
    else if (strcmp(params[0].key, WarningsList) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        // Warnings list acknowledgement - merge remote warnings
        processWarningsListAck(sender, params[0].key, params[0].value, params, paramCount);
    }
    else if (strcmp(params[0].key, RelayRetrieveStates) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        // Relay state acknowledgement - handle both formats:
        // 1. ACK:R2=ok (just acknowledgement, no relay state - paramCount == 1)
        // 2. ACK:R2=ok:0=0 (acknowledgement with relay state - paramCount == 2)
        
        if (paramCount >= 2)
        {
            // Format: ACK:R2=ok:0=0 (with relay index and state)
            if (!SystemFunctions::isAllDigits(params[1].key) || !SystemFunctions::isAllDigits(params[1].value))
            {
                return true;
            }

            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[1].key, nullptr, 0));
            bool isOn = SystemFunctions::parseBooleanValue(params[1].value);
            
            RelayStateUpdate update = { relayIndex, isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::RelayState), &update);
        }
    }
    else if (strcmp(params[0].key, RelayStatusGet) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        if (paramCount >= 2)
        {
            uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[1].key, nullptr, 0));
            bool isOn = SystemFunctions::parseBooleanValue(params[1].value);

            RelayStateUpdate update = { relayIndex, isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::RelayState), &update);
        }
        else
        {
			sendDebugMessage(F("Invalid R4 ACK format RelayStatusGet"), AckCommand);
        }
    }
    else if (strcmp(params[0].key, SoundSignalActive) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        if (paramCount >= 2)
        {
            bool isOn = SystemFunctions::parseBooleanValue(params[1].value);

            BoolStateUpdate update = { isOn };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::SoundSignal), &update);
        }
        else
        {
            sendDebugMessage(F("Invalid R4 ACK format Sound Signal Active"), AckCommand);
        }
    }
	else if (strcmp(params[0].key, SystemCpuUsage) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        if (paramCount >= 2)
        {
            uint8_t cpuUsage = static_cast<uint8_t>(strtoul(params[1].value, nullptr, 0));
			UInt8Update update = { cpuUsage };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CpuUsage), &update);
        }
        else
        {
            sendDebugMessage(F("Invalid F3 ACK format: cpu"), AckCommand);
        }
    }
    else if (strcmp(params[0].key, SystemFreeMemory) == 0 && strcmp(params[0].value, AckSuccess) == 0)
    {
        if (paramCount >= 2)
        {
            uint16_t freeMemory = static_cast<uint16_t>(strtoul(params[1].value, nullptr, 0));
            UInt16Update update = { freeMemory };
            notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::MemoryUsage), &update);
        }
        else
        {
            sendDebugMessage(F("Invalid F2 ACK format free memory"), AckCommand);
        }
	}
#endif

    return true;
}

const char* const* AckCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { AckCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}