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
#include "BroadcastManager.h"
#include "BaseCommandHandler.h"

constexpr unsigned long UpdateIntervalMs = 60000;

BroadcastManager::BroadcastManager(SerialCommandManager* computerSerial, SerialCommandManager* linkSerial)
	: _computerSerial(computerSerial), _linkSerial(linkSerial), _nextUpdateTime(0)
{
}

void BroadcastManager::update(unsigned long now)
{
    // Perform periodic updates every 1 minute: send configuration values to serial interfaces
	if (now > _nextUpdateTime)
    {
        _nextUpdateTime = now + UpdateIntervalMs;

		Config* config = ConfigManager::getConfigPtr();
        
        if (!config)
			return;
		char buffer[10];
		snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), config->hornRelayIndex);
        sendCommand(ConfigSoundRelayId, buffer, true);
		snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), static_cast<uint8_t>(config->vesselType));
        sendCommand(ConfigBoatType, buffer, true);
    }
}

void BroadcastManager::sendCommand(const char* command, const char* params, bool linkOnly)
{
    if (_computerSerial && !linkOnly)
    {
        _computerSerial->sendCommand(command, params);
    }

    if (_linkSerial)
    {
        _linkSerial->sendCommand(command, params);
    }
}

void BroadcastManager::sendCommand(const char* command, const char* message, const char* identifier, StringKeyValue* params, uint8_t argLength)
{
    if (_computerSerial)
    {
        _computerSerial->sendCommand(command, message, identifier, params, argLength);
    }
    if (_linkSerial)
    {
        _linkSerial->sendCommand(command, message, identifier, params, argLength);
    }
}

void BroadcastManager::sendCommand(const char* command, StringKeyValue* params, uint8_t argLength)
{
    sendCommand(command, "", "", params, argLength);
}

void BroadcastManager::sendDebug(const char* message, const char* source)
{
    if (_computerSerial)
    {
        _computerSerial->sendDebug(message, source);
    }
}

void BroadcastManager::sendError(const char* message, const char* source)
{
    if (_computerSerial)
    {
        _computerSerial->sendError(message, source);
    }
}
