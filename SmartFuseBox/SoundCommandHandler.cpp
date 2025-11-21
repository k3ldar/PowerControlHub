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
    SoundManager* soundManager)
	: _commandMgrComputer(commandMgrComputer), _commandMgrLink(commandMgrLink), _soundManager(soundManager)
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

bool SoundCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
{
	(void)params;

    if (_soundManager == nullptr)
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
        _soundManager->playSound(SoundType::None);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundIsActive)
    {
        StringKeyValue param = { String(static_cast<uint8_t>(_soundManager->getCurrentSoundType())), String(static_cast<uint8_t>(_soundManager->getCurrentSoundState())) };
        sendAckOk(sender, cmd, &param);
    }
    else if (cmd == SoundDangerSos)
    {
        _soundManager->playSound(SoundType::Sos);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundFog)
    {
        _soundManager->playSound(SoundType::Fog);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverAstern)
    {
        _soundManager->playSound(SoundType::MoveAstern);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverPort)
    {
        _soundManager->playSound(SoundType::MovePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverStarboard)
    {
        _soundManager->playSound(SoundType::MoveStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundManeuverDanger)
    {
        _soundManager->playSound(SoundType::MoveDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeConsent)
    {
        _soundManager->playSound(SoundType::OvertakeConsent);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeDanger)
    {
        _soundManager->playSound(SoundType::OvertakeDanger);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakePort)
    {
        _soundManager->playSound(SoundType::OvertakePort);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundOvertakeStarboard)
    {
        _soundManager->playSound(SoundType::OvertakeStarboard);
        sendAckOk(sender, cmd);
    }
    else if (cmd == SoundTest)
    {
        _soundManager->playSound(SoundType::Test);
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
