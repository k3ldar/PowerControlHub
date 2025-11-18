#include "WarningCommandHandler.h"



WarningCommandHandler::WarningCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
{
}

bool WarningCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
    String cmd = command;
    cmd.trim();

    // Ensure warning manager is available
    if (!_warningManager)
    {
		sendAckErr(sender, cmd, F("Warning manager not configured"));
        sendDebugMessage(F("Warning manager not available"), F("WarningCommandHandler"));
        return false;
    }

    if (cmd == WarningsActive && paramCount == 0)
    {
        uint8_t count = 0;
        for (uint8_t i = 1; i < WarningCount; i++)
        {
            WarningType type = static_cast<WarningType>(i);

            if (_warningManager->isWarningActive(type))
            {
                count++;
            }
        }

        StringKeyValue param = { ValueParamName, String(count) };
        sendAckOk(sender, cmd, &param);
        return true;
    }
    else if (cmd == WarningsList && paramCount == 0)
    {
        // Send list of all defined warning types with their active status

        for (uint8_t i = 1; i < WarningCount; i++)  // Start at 1 to skip WarningType::None
        {
            WarningType type = static_cast<WarningType>(i);
            bool isActive = _warningManager->isWarningActive(type);

            StringKeyValue param = { String(i), isActive ? "1" : "0" };
            sendAckOk(sender, cmd, &param);
        }

        return true;
    }
    else if (cmd == WarningStatus && paramCount == 1)
    {
        // Return warning status for specific warning (true if active otherwise false)
        // key will be warning type expressed as 0x04 etc, value is ignored on request and
        // returned as "1" or "0" in AckOk
        String key = params[0].key;
        key.trim();

        WarningType warningType = WarningType::None;

        // Parse and validate warning type
        if (!convertWarningTypeFromString(key, warningType))
        {
			sendAckErr(sender, cmd, F("Invalid warning type"));
            return true;
        }

        bool isActive = _warningManager->isWarningActive(warningType);

        StringKeyValue param = { key, isActive ? "1" : "0" };
        sendAckOk(sender, cmd, &param);

        return true;
    }
    else if (cmd == WarningsClear && paramCount == 0)
    {
        _warningManager->clearAllWarnings();

        sendAckOk(sender, cmd);
        return true;
    }
    else if (cmd == WarningsAdd && paramCount == 1)
    {
        String key = params[0].key;
        key.trim();
        String val = params[0].value;
        val.trim();

        WarningType warningType = WarningType::None;

        // Parse and validate warning type
        if (!convertWarningTypeFromString(key, warningType))
        {
			sendAckErr(sender, cmd, F("Invalid warning type"));
            return true;
        }

        bool isActive = parseBooleanValue(val);

        if (isActive)
            _warningManager->raiseWarning(warningType);
        else
            _warningManager->clearWarning(warningType);

		sendAckOk(sender, cmd, &params[0]);
        return true;
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Warning command"), F("WarningCommandHandler"));
        return false;
    }
}

bool WarningCommandHandler::convertWarningTypeFromString(const String& str, WarningType& outType)
{
    uint8_t warningTypeInt = 0;

    // Parse the string based on format
    if (str.startsWith(F("0x")) || str.startsWith(F("0X")))
    {
        // Parse hexadecimal (skip the "0x" prefix)
        warningTypeInt = strtoul(str.c_str() + 2, nullptr, 16);
    }
    else if (isAllDigits(str))
    {
        // Parse decimal
        warningTypeInt = str.toInt();
    }
    else
    {
        sendDebugMessage(F("Invalid warning type format"), F("WarningCommandHandler"));
        return false;
    }

    // Validate range: must be between 1 and WarningCount-1 (exclude None)
    if (warningTypeInt == 0 || warningTypeInt >= WarningCount)
    {
        sendDebugMessage(F("Warning type out of range"), F("WarningCommandHandler"));
        return false;
    }

    outType = static_cast<WarningType>(warningTypeInt);
    return true;
}

const String* WarningCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { WarningsActive, WarningsList, WarningStatus,
        WarningsClear, WarningsAdd };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}