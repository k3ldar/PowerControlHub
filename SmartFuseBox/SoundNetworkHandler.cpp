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

#include "Local.h"
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

void SoundNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
	char buffer[MaximumJsonResponseBufferSize];
	buffer[0] = '\0';

	formatStatusJson(buffer, sizeof(buffer));
	client->print(buffer);
}
