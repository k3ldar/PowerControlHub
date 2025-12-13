
#include "SoundNetworkHandler.h"

SoundNetworkHandler::SoundNetworkHandler(SoundController* soundController)
	: _soundController(soundController)
{
}

CommandResult SoundNetworkHandler::handleRequest(const char* method,
	const char* command,
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

	// none of the sound commands should receive any parameters
	if (paramCount > 0)
	{
		formatJsonResponse(responseBuffer, bufferSize, false, "Invalid parameters");
		return CommandResult::error(InvalidCommandParameters);
	}

	if (strcmp(command, SoundSignalCancel) == 0)
	{
		_soundController->playSound(SoundType::None);
	}
	else if (strcmp(command, SoundSignalActive) == 0)
	{
		// status is always returned below
	}
	else if (strcmp(command, SoundSignalSoS) == 0)
	{
		_soundController->playSound(SoundType::Sos);
	}
	else if (strcmp(command, SoundSignalFog) == 0)
	{
		_soundController->playSound(SoundType::Fog);
	}
	else if (strcmp(command, SoundSignalMoveAstern) == 0)
	{
		_soundController->playSound(SoundType::MoveAstern);
	}
	else if (strcmp(command, SoundSignalMovePort) == 0)
	{
		_soundController->playSound(SoundType::MovePort);
	}
	else if (strcmp(command, SoundSignalMoveStarboard) == 0)
	{
		_soundController->playSound(SoundType::MoveStarboard);
	}
	else if (strcmp(command, SoundSignalMoveDanger) == 0)
	{
		_soundController->playSound(SoundType::MoveDanger);
	}
	else if (strcmp(command, SoundSignalOvertakeConsent) == 0)
	{
		_soundController->playSound(SoundType::OvertakeConsent);
	}
	else if (strcmp(command, SoundSignalOvertakeDanger) == 0)
	{
		_soundController->playSound(SoundType::OvertakeDanger);
	}
	else if (strcmp(command, SoundSignalOvertakePort) == 0)
	{
		_soundController->playSound(SoundType::OvertakePort);
	}
	else if (strcmp(command, SoundSignalOvertakeStarboard) == 0)
	{
		_soundController->playSound(SoundType::OvertakeStarboard);
	}
	else if (strcmp(command, SoundSignalTest) == 0)
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
