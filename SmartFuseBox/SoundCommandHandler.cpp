// 
// 
// 

#include "SoundCommandHandler.h"



SoundCommandHandler::SoundCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, 
    SoundController* soundController)
	: _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink), _soundController(soundController)
{

}

const String* SoundCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SoundSignalCancel, SoundSignalActive, SoundSignalSoS, 
        SoundSignalFog, SoundSignalMoveStarboard, SoundSignalMovePort, SoundSignalMoveAstern, 
        SoundSignalMoveDanger, SoundSignalOvertakeStarboard, SoundSignalOvertakePort, 
        SoundSignalOvertakeConsent, SoundSignalOvertakeDanger, SoundSignalTest };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}

bool SoundCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
{
	(void)params;

    if (_soundController == nullptr)
    {
        sendAckErr(sender, command, "Sound manager not initialized");
		return true;
    }

    String cmd = command;
    cmd.trim();

    // none of the sound commands should receive any parameters
    if (paramCount > 0)
    {
        sendAckErr(sender, cmd, "Invalid Parameters");
        return true;
    }

    if (cmd == SoundSignalCancel)
    {
        _soundController->playSound(SoundType::None);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalActive)
    {
        StringKeyValue param = { String(static_cast<uint8_t>(_soundController->getCurrentSoundType())), String(static_cast<uint8_t>(_soundController->getCurrentSoundState())) };
        sendAckOk(sender, cmd, &param);
    }
    else if (cmd == SoundSignalSoS)
    {
        _soundController->playSound(SoundType::Sos);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalFog)
    {
        _soundController->playSound(SoundType::Fog);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalMoveAstern)
    {
        _soundController->playSound(SoundType::MoveAstern);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalMovePort)
    {
        _soundController->playSound(SoundType::MovePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalMoveStarboard)
    {
        _soundController->playSound(SoundType::MoveStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalMoveDanger)
    {
        _soundController->playSound(SoundType::MoveDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalOvertakeConsent)
    {
        _soundController->playSound(SoundType::OvertakeConsent);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalOvertakeDanger)
    {
        _soundController->playSound(SoundType::OvertakeDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalOvertakePort)
    {
        _soundController->playSound(SoundType::OvertakePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalOvertakeStarboard)
    {
        _soundController->playSound(SoundType::OvertakeStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundSignalTest)
    {
        _soundController->playSound(SoundType::Test);
        sendAckOk(sender, cmd);
    }
    else
    {
        sendAckErr(sender, cmd, F("Unknown system command"));
    }

    broadcast(cmd);

    return true;
}

void SoundCommandHandler::broadcast(const String& cmd, const StringKeyValue* param)
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
