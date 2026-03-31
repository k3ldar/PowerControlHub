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
#include "WarningCommandHandler.h"
#include "SystemFunctions.h"

WarningCommandHandler::WarningCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcastManager, warningManager)
{
}

bool WarningCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
	WarningManager* warningManager = getWarningManager();

    // Ensure warning manager is available
    if (!warningManager)
    {
        sendAckErr(sender, command, F("Warning manager not configured"));
        sendDebugMessage(F("Warning manager not available"), F("WarningCommandHandler"));
        return false;
    }

    if (SystemFunctions::commandMatches(command, WarningsActive) && paramCount == 0)
    {
        // Return the active warnings as a bitmask value
        uint32_t activeWarnings = warningManager->getActiveWarningsMask();
        char warningsHex[32];
        snprintf_P(warningsHex, sizeof(warningsHex), PSTR("%s%lx"), HexPrefix, activeWarnings);
        StringKeyValue param = makeParam(ValueParamName, warningsHex);
        sendAckOk(sender, command, &param);
        return true;
    }
    else if (SystemFunctions::commandMatches(command, WarningsList) && paramCount == 0)
    {
        // Return the complete bitmask of active warnings as a single value
        uint32_t activeWarnings = warningManager->getActiveWarningsMask();
        char warningsHex[32];
        snprintf_P(warningsHex, sizeof(warningsHex), PSTR("%s%lx"), HexPrefix, activeWarnings);
        StringKeyValue param = makeParam(ValueParamName, warningsHex);
        sendAckOk(sender, command, &param);
        return true;
    }
    else if (SystemFunctions::commandMatches(command, WarningStatus) && paramCount == 1)
    {
        // Return warning status for specific warning (true if active otherwise false)
        // key will be warning type expressed as 0x04 etc, value is ignored on request and
        // returned as "1" or "0" in AckOk

        WarningType warningType = WarningType::None;

        // Parse and validate warning type
        if (!convertWarningTypeFromString(params[0].key, warningType))
        {
            sendAckErr(sender, command, F("Invalid warning type"));
            return true;
        }

        bool isActive = warningManager->isWarningActive(warningType);

        StringKeyValue param;
        strncpy(param.key, params[0].key, sizeof(param.key));
        strncpy(param.value, (isActive ? "1" : "0"), sizeof(param.value));
        sendAckOk(sender, command, &param);

        return true;
    }
    else if (SystemFunctions::commandMatches(command, WarningsClear) && paramCount == 0)
    {
        warningManager->clearAllWarnings();

        sendAckOk(sender, command);
        return true;
    }
    else if (SystemFunctions::commandMatches(command, WarningsAdd) && paramCount == 1)
    {
        WarningType warningType = WarningType::None;

        // Parse and validate warning type
        if (!convertWarningTypeFromString(params[0].key, warningType))
        {
            sendAckErr(sender, command, F("Invalid warning type"));
            return true;
        }

        bool isActive = SystemFunctions::parseBooleanValue(params[0].value);

        if (isActive)
            warningManager->raiseWarning(warningType);
        else
            warningManager->clearWarning(warningType);

        sendAckOk(sender, command, &params[0]);
        return true;
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Warning command"), F("WarningCommandHandler"));
        return false;
    }
}

bool WarningCommandHandler::convertWarningTypeFromString(const char* str, WarningType& outType)
{
    uint32_t warningTypeInt = 0;

    // Parse the string based on format
    if (SystemFunctions::startsWith(str, "0x") || SystemFunctions::startsWith(str, "0X"))
    {
        // Parse hexadecimal (skip the "0x" prefix)
        warningTypeInt = strtoul(str + 2, nullptr, 16);
    }
    else if (SystemFunctions::isAllDigits(str))
    {
        // Parse decimal
        warningTypeInt = static_cast<uint32_t>(strtoul(str, nullptr, 0));
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

const char* const* WarningCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { WarningsActive, WarningsList, WarningStatus,
        WarningsClear, WarningsAdd };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}