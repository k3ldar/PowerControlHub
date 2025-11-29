#pragma once
#include <Arduino.h>
#include <SerialCommandManager.h>
#include "ConfigManager.h"
#include "SharedConstants.h"

/**
 * @brief Centralized manager for broadcasting commands to multiple serial ports.
 *
 * Simplifies sending commands/debug/errors to both computer (debug) and link (relay controller)
 * serial connections simultaneously.
 */
class BroadcastManager
{
private:
    SerialCommandManager* _computerSerial;
    SerialCommandManager* _linkSerial;

	unsigned long _nextUpdateTime;

public:
    /**
     * @brief Construct a broadcast manager with two serial command managers.
     *
     * @param computerSerial Pointer to computer serial (debug) manager (can be nullptr)
     * @param linkSerial Pointer to link serial (relay controller) manager (can be nullptr)
     */
    BroadcastManager(SerialCommandManager* computerSerial, SerialCommandManager* linkSerial);

    /**
     * @brief allows for specific updates at regulated intervals.
     *
	 * @param now current time in milliseconds
     */
    void update(unsigned long now);

    /**
     * @brief Send a command to all registered serial managers.
     *
     * @param command The command string to send
     * @param params Optional parameters string (default: empty string)
     */
    void sendCommand(const char* command, const char* params = "", bool linkOnly = false);

    /**
     * @brief Send a command to all registered serial managers (String overload for convenience).
     *
     * @param command The command string to send
     * @param params Optional parameters string
     */
    void sendCommand(const String& command, const String& params = "", bool linkOnly = false);

    /**
     * @brief Send a command to all registered serial managers.
     *
	 * @param command The command
	 * @param message The command message (default: empty string)
	 * @param identifier Optional identifier (default: empty string)
	 * @param params Optional array of key/value parameters (default: nullptr)
	 * @param argLength Number of parameters in the params array (default: 0)
     */
    void sendCommand(const char* command, char* message, char* identifier, StringKeyValue* params = nullptr, uint8_t argLength = 0);

    /**
     * @brief Send a command to all registered serial managers.
     *
     * @param command The command
     * @param params Optional array of key/value parameters
     * @param argLength Number of parameters in the params array
     */
    void sendCommand(const char* command, StringKeyValue* params, uint8_t argLength);

    /**
     * @brief Send a debug message to all registered serial managers.
     *
     * @param message The debug message
     * @param source Optional source identifier (default: empty string)
     */
    void sendDebug(const char* message, const char* source = "");

    /**
     * @brief Send a debug message to all registered serial managers (String overload).
     *
     * @param message The debug message
     * @param source Optional source identifier
     */
    void sendDebug(const String& message, const String& source = "");

    /**
     * @brief Send an error message to all registered serial managers.
     *
     * @param message The error message
     * @param source Optional source identifier (default: empty string)
     */
    void sendError(const char* message, const char* source = "");

    /**
     * @brief Send an error message to all registered serial managers (String overload).
     *
     * @param message The error message
     * @param source Optional source identifier
     */
    void sendError(const String& message, const String& source = "");

    /**
     * @brief Get pointer to computer serial manager (for selective operations).
     */
    SerialCommandManager* getComputerSerial() const { return _computerSerial; }

    /**
     * @brief Get pointer to link serial manager (for selective operations).
     */
    SerialCommandManager* getLinkSerial() const { return _linkSerial; }
};