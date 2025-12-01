// 
// 
// 

#include "SoundCommandHandler.h"


constexpr char SoundCancellAll[] = "H0";
constexpr char SoundIsActive[] = "H1";
constexpr char SoundDangerSos[] = "H2";
constexpr char SoundFog[] = "H3";
constexpr char SoundManeuverStarboard[] = "H4";
constexpr char SoundManeuverPort[] = "H5";
constexpr char SoundManeuverAstern[] = "H6";
constexpr char SoundManeuverDanger[] = "H7";
constexpr char SoundOvertakeStarboard[] = "H8";
constexpr char SoundOvertakePort[] = "H9";
constexpr char SoundOvertakeConsent[] = "H10";
constexpr char SoundOvertakeDanger[] = "H11";
constexpr char SoundTest[] = "H12";

SoundCommandHandler::SoundCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, 
    SoundController* soundController)
	: _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink), _soundController(soundController)
{

}

const String* SoundCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SoundCancellAll, SoundIsActive, SoundDangerSos, SoundFog,
        SoundManeuverAstern, SoundManeuverDanger, SoundManeuverPort, SoundManeuverStarboard, 
        SoundOvertakeConsent, SoundOvertakeDanger, SoundOvertakePort, SoundOvertakeStarboard, 
        SoundTest };
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

    if (cmd == SoundCancellAll)
    {
        _soundController->playSound(SoundType::None);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundIsActive)
    {
        StringKeyValue param = { String(static_cast<uint8_t>(_soundController->getCurrentSoundType())), String(static_cast<uint8_t>(_soundController->getCurrentSoundState())) };
        sendAckOk(sender, cmd, &param);
    }
    else if (cmd == SoundDangerSos)
    {
        _soundController->playSound(SoundType::Sos);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundFog)
    {
        _soundController->playSound(SoundType::Fog);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverAstern)
    {
        _soundController->playSound(SoundType::MoveAstern);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverPort)
    {
        _soundController->playSound(SoundType::MovePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverStarboard)
    {
        _soundController->playSound(SoundType::MoveStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverDanger)
    {
        _soundController->playSound(SoundType::MoveDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeConsent)
    {
        _soundController->playSound(SoundType::OvertakeConsent);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeDanger)
    {
        _soundController->playSound(SoundType::OvertakeDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakePort)
    {
        _soundController->playSound(SoundType::OvertakePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeStarboard)
    {
        _soundController->playSound(SoundType::OvertakeStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundTest)
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
