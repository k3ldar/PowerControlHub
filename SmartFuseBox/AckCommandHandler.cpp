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
#include "AckCommandHandler.h"
#include "SystemFunctions.h"
#include "ConfigController.h"

const char AckCommand[] = "ACK";

AckCommandHandler::AckCommandHandler(BroadcastManager* broadcastManager, 
    MessageBus* messageBus,
    WarningManager* warningManager)
    : BaseNextionCommandHandler(broadcastManager, messageBus, warningManager)
{

}


void AckCommandHandler::setConfigController(ConfigController* configController)
{
    _configController = configController;
}

bool AckCommandHandler::processConfigAck(SerialCommandManager* sender, const char* key, const char* value)
{
	(void)sender;

    // Handle C1 (ConfigGetSettings) acknowledgement
    if (strcmp(key, ConfigGetSettings) == 0 && strcmp(value, AckSuccess) == 0)
    {
        sendDebugMessage(F("Config sync started"), AckCommand);
        return true;
    }

    // All other config ACKs (C3-C27) are handled by ConfigCommandHandler
    // Just acknowledge them here
    return false;
}

bool AckCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    (void)sender;

    // Validate command
    if (!SystemFunctions::commandMatches(command, AckCommand))
    {
        char debugMsg[64];
        snprintf_P(debugMsg, sizeof(debugMsg), PSTR("Unknown ACK command %s"), command);
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
    if (SystemFunctions::commandMatches(params[0].key, AckCommand))
    {
        return true; // Silently ignore, consider it handled
    }

    if (!SystemFunctions::commandMatches(params[0].value, AckSuccess))
    {
        char debugMsg[120];
        snprintf_P(debugMsg, sizeof(debugMsg), PSTR("ACK indicates failure: key='%s', val='%s'"), params[0].key, params[0].value);
        _broadcaster->sendDebug(debugMsg, AckCommand);
        return false;
    }

    // only process known ACK keys if you need to take action

#if defined(NEXTION_DISPLAY_DEVICE)
	if (strcmp(params[0].key, RelayRetrieveStates) == 0 && strcmp(params[0].value, AckSuccess) == 0)
	{
		if (paramCount >= 2)
		{
			if (!SystemFunctions::isAllDigits(params[1].key) || !SystemFunctions::isAllDigits(params[1].value))
			{
				sendDebugMessage(F("Invalid R2 Ack response"), F("AckCommandHandler"));
				return true;
			}

			uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[1].key, nullptr, 0));
			bool isOn = SystemFunctions::parseBooleanValue(params[1].value);
			uint8_t changedMask = 1 << relayIndex;
			(void)isOn;

			if (_messageBus)
				_messageBus->publish<RelayStatusChanged>(changedMask);
		}
	}
	else if (strcmp(params[0].key, RelayStatusGet) == 0 && strcmp(params[0].value, AckSuccess) == 0)
	{
		if (paramCount >= 2)
		{
			uint8_t relayIndex = static_cast<uint8_t>(strtoul(params[1].key, nullptr, 0));
			bool isOn = SystemFunctions::parseBooleanValue(params[1].value);
			uint8_t changedMask = 1 << relayIndex;

			(void)isOn;
			if (_messageBus)
				_messageBus->publish<RelayStatusChanged>(changedMask);
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
			if (_messageBus)
				_messageBus->publish<SoundSignalUpdated>(isOn);
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
			if (_messageBus)
				_messageBus->publish<CpuUsageUpdated>(cpuUsage);
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
			if (_messageBus)
				_messageBus->publish<MemoryUsageUpdated>(freeMemory);
		}
		else
		{
			sendDebugMessage(F("Invalid F2 ACK format free memory"), AckCommand);
		}
	}
#else
	processConfigAck(sender, params[0].key, params[0].value);
#endif

	return true;
}

const char* const* AckCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { AckCommand };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}