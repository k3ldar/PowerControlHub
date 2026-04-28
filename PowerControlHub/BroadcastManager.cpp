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

BroadcastManager::BroadcastManager(SerialCommandManager* computerSerial)
	: _computerSerial(computerSerial)
{
}

void BroadcastManager::sendCommand(const char* command, const char* params)
{
    if (_computerSerial)
    {
        _computerSerial->sendCommand(command, params);
    }
}

void BroadcastManager::sendCommand(const char* command, const char* message, const char* identifier, StringKeyValue* params, uint8_t argLength)
{
    if (_computerSerial)
    {
        _computerSerial->sendCommand(command, message, identifier, params, argLength);
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
