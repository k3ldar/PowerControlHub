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
#pragma once

#include <Arduino.h>
#include "BaseCommandHandler.h"
#include "BroadcastManager.h"
#include "WarningManager.h"

/**
 * @brief Base class for command handlers that need broadcasting capabilities.
 * 
 * This class extends BaseCommandHandler with BroadcastManager dependency and helper methods
 * for sending commands, debug messages, and errors. It provides a minimal shared foundation
 * that can be used across different contexts (boat-specific, shared, etc.).
 * 
 * Handlers that only need broadcasting capabilities should inherit from this class.
 * For boat-specific handlers that need additional dependencies (NextionControl, WarningManager),
 * use BaseBoatCommandHandler instead.
 */
class SharedBaseCommandHandler : public virtual BaseCommandHandler
{
private:
    WarningManager* _warningManager;

protected:
	BroadcastManager* _broadcaster;

    /**
     * @brief Constructor with broadcast dependency.
     * 
     * @param broadcaster Manager for broadcasting commands/debug/errors
     */
    SharedBaseCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager)
		: _warningManager(warningManager), _broadcaster(broadcaster)
    {
    };

    /**
     * @brief Get pointer to the WarningManager.
     * 
     * @return WarningManager* Pointer to the warning manager (can be nullptr)
	 */
    WarningManager* getWarningManager() const
    {
        return _warningManager;
	}

    /**
     * @brief Send a debug message to the computer serial (via BroadcastManager).
     * 
     * @param message Debug message content
     * @param identifier Command handler identifier
     */
    void sendDebugMessage(const __FlashStringHelper* message, const char* identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendDebug(reinterpret_cast<const char*>(message), identifier);
        }
    }

    /**
     * @brief Send a debug message to the computer serial (via BroadcastManager).
     *
     * @param message Debug message content
     * @param identifier Command handler identifier
     */
    void sendDebugMessage(const __FlashStringHelper* message, const __FlashStringHelper* identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendDebug(reinterpret_cast<const char*>(message), reinterpret_cast<const char*>(identifier));
        }
    }

    /**
     * @brief Send a debug message to the computer serial (via BroadcastManager).
     * 
     * @param message Debug message content (const char* for efficiency)
     * @param identifier Command handler identifier
     */
    void sendDebugMessage(const char* message, const char* identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendDebug(message, identifier);
        }
    }

    /**
     * @brief Send an error message to the computer serial (via BroadcastManager).
     * 
     * @param message Error message content
     * @param identifier Command handler identifier
     */
    void sendErrorMessage(const __FlashStringHelper* message, const char* identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendError(reinterpret_cast<const char*>(message), identifier);
        }
    }

    void sendErrorMessage(const char* message, const char* identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendError(message, identifier);
        }
    }
};