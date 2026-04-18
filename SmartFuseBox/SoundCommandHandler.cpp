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
// 
// 
// 

#include "SoundCommandHandler.h"
#include "SystemFunctions.h"



SoundCommandHandler::SoundCommandHandler(SerialCommandManager* commandMgrComputer,
	SoundController* soundController)
	: _commandMgrComputer(commandMgrComputer), _soundController(soundController)
{

}

const char* const* SoundCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { SoundSignalCancel, SoundSignalActive, SoundSignalSoS,
        SoundSignalFog, SoundSignalMoveStarboard, SoundSignalMovePort, SoundSignalMoveAstern, 
        SoundSignalMoveDanger, SoundSignalOvertakeStarboard, SoundSignalOvertakePort, 
        SoundSignalOvertakeConsent, SoundSignalOvertakeDanger, SoundSignalTest };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SoundCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
	(void)params;

    if (_soundController == nullptr)
    {
        sendAckErr(sender, command, F("Sound manager not initialized"));
		return true;
    }

    // none of the sound commands should receive any parameters
    if (paramCount > 0)
    {
        sendAckErr(sender, command, F("Invalid Parameters"));
        return true;
    }

    if (SystemFunctions::commandMatches(command, SoundSignalCancel))
    {
        _soundController->playSound(SoundType::None);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalActive))
    {
        StringKeyValue param = makeParam(static_cast<uint8_t>(_soundController->getCurrentSoundType()), static_cast<uint8_t>(_soundController->getCurrentSoundState()));
        sendAckOk(sender, command, &param);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalSoS))
    {
        _soundController->playSound(SoundType::Sos);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalFog))
    {
        _soundController->playSound(SoundType::Fog);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalMoveAstern))
    {
        _soundController->playSound(SoundType::MoveAstern);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalMovePort))
    {
        _soundController->playSound(SoundType::MovePort);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalMoveStarboard))
    {
        _soundController->playSound(SoundType::MoveStarboard);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalMoveDanger))
    {
        _soundController->playSound(SoundType::MoveDanger);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalOvertakeConsent))
    {
        _soundController->playSound(SoundType::OvertakeConsent);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalOvertakeDanger))
    {
        _soundController->playSound(SoundType::OvertakeDanger);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalOvertakePort))
    {
        _soundController->playSound(SoundType::OvertakePort);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalOvertakeStarboard))
    {
        _soundController->playSound(SoundType::OvertakeStarboard);
        sendAckOk(sender, command);
    }
    else if (SystemFunctions::commandMatches(command, SoundSignalTest))
    {
        _soundController->playSound(SoundType::Test);
        sendAckOk(sender, command);
    }
    else
    {
        sendAckErr(sender, command, F("Unknown system command"));
    }

    broadcast(command);

    return true;
}

void SoundCommandHandler::broadcast(const char* cmd, const StringKeyValue* param)
{
    if (_commandMgrComputer != nullptr)
    {
        sendAckOk(_commandMgrComputer, cmd, param);
    }
}
