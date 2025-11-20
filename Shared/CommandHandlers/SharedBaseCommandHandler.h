#pragma once

#include <Arduino.h>
#include "BaseCommandHandler.h"
#include "BroadcastManager.h"

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
class SharedBaseCommandHandler : public BaseCommandHandler
{
protected:
	BroadcastManager* _broadcaster;

    /**
     * @brief Constructor with broadcast dependency.
     * 
     * @param broadcaster Manager for broadcasting commands/debug/errors
     */
    SharedBaseCommandHandler(BroadcastManager* broadcaster)
		: _broadcaster(broadcaster)
    {
    };

    /**
     * @brief Send a debug message to the computer serial (via BroadcastManager).
     * 
     * @param message Debug message content
     * @param identifier Command handler identifier
     */
    void sendDebugMessage(const String& message, const String& identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendDebug(message, identifier);
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
    void sendErrorMessage(const String& message, const String& identifier)
    {
        if (_broadcaster)
        {
            _broadcaster->sendError(message, identifier);
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