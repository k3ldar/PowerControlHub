
#include "SoundNetworkHandler.h"

SoundNetworkHandler::SoundNetworkHandler(SoundController* soundController)
	: _soundController(soundController)
{
}

CommandResult SoundNetworkHandler::handleRequest(const String& method,
	const String& command,
	StringKeyValue* params,
	uint8_t paramCount,
	char* responseBuffer,
	size_t bufferSize)
{
	(void)method;
	(void)params;

	if (_soundController == nullptr)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Sound controller not initialized");
		return CommandResult::error(SoundControllerNotInitialised);
	}

	String cmd = command;
	cmd.trim();

	// none of the sound commands should receive any parameters
	if (paramCount > 0)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Invalid parameters");
		return CommandResult::error(InvalidCommandParameters);
	}

	if (cmd == SoundSignalCancel)
	{
		_soundController->playSound(SoundType::None);
	}
	else if (cmd == SoundSignalActive)
	{
		// status is always returned below
	}
	else if (cmd == SoundSignalSoS)
	{
		_soundController->playSound(SoundType::Sos);
	}
	else if (cmd == SoundSignalFog)
	{
		_soundController->playSound(SoundType::Fog);
	}
	else if (cmd == SoundSignalMoveAstern)
	{
		_soundController->playSound(SoundType::MoveAstern);
	}
	else if (cmd == SoundSignalMovePort)
	{
		_soundController->playSound(SoundType::MovePort);
	}
	else if (cmd == SoundSignalMoveStarboard)
	{
		_soundController->playSound(SoundType::MoveStarboard);
	}
	else if (cmd == SoundSignalMoveDanger)
	{
		_soundController->playSound(SoundType::MoveDanger);
	}
	else if (cmd == SoundSignalOvertakeConsent)
	{
		_soundController->playSound(SoundType::OvertakeConsent);
	}
	else if (cmd == SoundSignalOvertakeDanger)
	{
		_soundController->playSound(SoundType::OvertakeDanger);
	}
	else if (cmd == SoundSignalOvertakePort)
	{
		_soundController->playSound(SoundType::OvertakePort);
	}
	else if (cmd == SoundSignalOvertakeStarboard)
	{
		_soundController->playSound(SoundType::OvertakeStarboard);
	}
	else if (cmd == SoundSignalTest)
	{
		_soundController->playSound(SoundType::Test);
	}
	else
	{
		return CommandResult::error(InvalidCommandParameters);
	}

	formatStatusJson(responseBuffer, bufferSize);
	return CommandResult::ok();
}

void SoundNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	snprintf(buffer, size, "\"sound\":{\"active\": %d,\"type\": %d}", 
		_soundController->isPlaying(), static_cast<int>(_soundController->getCurrentSoundType()));
}

void SoundNetworkHandler::formatWifiStatusJson(WiFiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
