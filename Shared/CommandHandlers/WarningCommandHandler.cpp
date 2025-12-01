#include "WarningCommandHandler.h"

#if defined(BOAT_CONTROL_PANEL)
// Constructor: pass the NextionControl pointer so we can notify the current page
WarningCommandHandler::WarningCommandHandler(BroadcastManager* broadcastManager,
    NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
#elif defined(FUSE_BOX_CONTROLLER)
WarningCommandHandler::WarningCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcastManager, warningManager)
#endif
{
}

bool WarningCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
{
    String cmd = command;
    cmd.trim();

	WarningManager* warningManager = getWarningManager();

    // Ensure warning manager is available
    if (!warningManager)
    {
        sendAckErr(sender, cmd, F("Warning manager not configured"));
        sendDebugMessage(F("Warning manager not available"), F("WarningCommandHandler"));
        return false;
    }

    if (cmd == WarningsActive && paramCount == 0)
    {
        // Return the active warnings as a bitmask value
        uint32_t activeWarnings = warningManager->getActiveWarningsMask();
        
        StringKeyValue param = { ValueParamName, String(activeWarnings, HEX) };
        sendAckOk(sender, cmd, &param);
        return true;
    }
    else if (cmd == WarningsList && paramCount == 0)
    {
        // Return the complete bitmask of active warnings as a single value
        uint32_t activeWarnings = warningManager->getActiveWarningsMask();
        
        StringKeyValue param = { ValueParamName, HexPrefix + String(activeWarnings, HEX)};
        sendAckOk(sender, cmd, &param);
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

        bool isActive = warningManager->isWarningActive(warningType);

        StringKeyValue param = { key, isActive ? "1" : "0" };
        sendAckOk(sender, cmd, &param);

        return true;
    }
    else if (cmd == WarningsClear && paramCount == 0)
    {
        warningManager->clearAllWarnings();

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
            warningManager->raiseWarning(warningType);
        else
            warningManager->clearWarning(warningType);

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
    uint32_t warningTypeInt = 0;

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

    // Validate: must be a valid power of 2 (single bit flag) and non-zero
    if (warningTypeInt == 0)
    {
        sendDebugMessage(F("Warning type cannot be None"), F("WarningCommandHandler"));
        return false;
    }

    // Check if it's a valid single-bit flag (power of 2)
    // A number is a power of 2 if (n & (n-1)) == 0
    if ((warningTypeInt & (warningTypeInt - 1)) != 0)
    {
        sendDebugMessage(F("Warning type must be a single bit flag"), F("WarningCommandHandler"));
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