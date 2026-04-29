/*
 * PowerControlHub
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
#include <SerialCommandManager.h>
#include "BroadcastManager.h"

/**
 * @brief Mixin class providing standardized logging functionality.
 * 
 * Can be used standalone or via multiple inheritance to add logging
 * capabilities to any class.
 */
class LoggingSupport
{
protected:
    /**
     * @brief Send a debug message via provided manager.
     */
    static void sendDebug(SerialCommandManager* mgr, const char* message, const char* source = "")
    {
        if (mgr)
        {
            mgr->sendDebug(message, source);
        }
    }
    
    /**
     * @brief Send an error message via provided manager.
     */
    static void sendError(SerialCommandManager* mgr, const char* message, const char* source = "")
    {
        if (mgr)
        {
            mgr->sendError(message, source);
        }
    }
    
    static void sendCommand(SerialCommandManager* mgr, const char* command, const char* params = "")
    {
        if (mgr)
        {
            mgr->sendCommand(command, params);
        }
	}

    static void sendCommand(SerialCommandManager* mgr, char* header, char* message, char* identifier, StringKeyValue* params = nullptr, uint8_t argLength = 0)
    {
        if (mgr)
        {
            mgr->sendCommand(header, message, identifier, params, argLength);
        }
    }
};

/**
 * @brief Specialization for classes with a single logger.
 */
class SingleLoggerSupport : public LoggingSupport
{
private:
    SerialCommandManager* _logger;
    
protected:
    explicit SingleLoggerSupport(SerialCommandManager* logger = nullptr) 
        : _logger(logger)
    {
    }
    
    void sendDebug(const char* message, const char* source = "")
    {
        if (_logger)
        {
            LoggingSupport::sendDebug(_logger, message, source);
        }
    }

	void sendDebug(const __FlashStringHelper* message, const __FlashStringHelper* source = nullptr)
    {
        if (_logger)
        {
            _logger->sendDebug(message, source);
        }
    }

    void sendDebug(const char* message, const __FlashStringHelper* source = nullptr)
    {
        if (_logger)
        {
            _logger->sendDebug(message, source);
        }
    }

    void sendError(const char* message, const char* source = "")
    {
        if (_logger)
        {
            LoggingSupport::sendError(_logger, message, source);
        }
    }

    void sendError(const __FlashStringHelper* message, const __FlashStringHelper* source = nullptr)
    {
        if (_logger)
        {
            _logger->sendError(message, source);
        }
    }

    void sendError(const char* message, const __FlashStringHelper* source = nullptr)
    {
        if (_logger)
        {
            _logger->sendError(message, source);
        }
    }
};

/**
 * @brief Specialization for classes with BroadcastManager.
 */
class BroadcastLoggerSupport : public LoggingSupport
{
private:
    BroadcastManager* _broadcaster;
    
protected:
    explicit BroadcastLoggerSupport(BroadcastManager* broadcaster = nullptr)
        : _broadcaster(broadcaster)
    {
    }
    
    void sendDebug(const char* message, const char* source = "")
    {
        if (_broadcaster)
        {
            _broadcaster->sendDebug(message, source);
        }
    }
    
    void sendError(const char* message, const char* source = "")
    {
        if (_broadcaster)
        {
            _broadcaster->sendError(message, source);
        }
    }

    void sendCommand(const char* command, StringKeyValue* params, uint8_t paramCount)
    {
        if (_broadcaster)
        {
            _broadcaster->sendCommand(command, params, paramCount);
        }
    }
};