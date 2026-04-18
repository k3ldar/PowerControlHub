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
#include <SerialCommandManager.h>
#include "ConfigManager.h"
#include "SystemDefinitions.h"

/**
 * @brief Centralized manager for broadcasting commands to the computer serial port.
 */
class BroadcastManager
{
private:
	SerialCommandManager* _computerSerial;

public:
	/**
	 * @brief Construct a broadcast manager.
	 *
	 * @param computerSerial Pointer to computer serial (debug) manager (can be nullptr)
	 */
	explicit BroadcastManager(SerialCommandManager* computerSerial);

    /**
     * @brief Send a command to all registered serial managers.
     *
     * @param command The command string to send
     * @param params Optional parameters string (default: empty string)
     */
    void sendCommand(const char* command, const char* params = "");

    /**
     * @brief Send a command to all registered serial managers.
     *
	 * @param command The command
	 * @param message The command message (default: empty string)
	 * @param identifier Optional identifier (default: empty string)
	 * @param params Optional array of key/value parameters (default: nullptr)
	 * @param argLength Number of parameters in the params array (default: 0)
     */
    void sendCommand(const char* command, const char* message, const char* identifier, StringKeyValue* params = nullptr, uint8_t argLength = 0);

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
     * @brief Send an error message to all registered serial managers.
     *
     * @param message The error message
     * @param source Optional source identifier (default: empty string)
     */
    void sendError(const char* message, const char* source = "");

    /**
     * @brief Get pointer to computer serial manager (for selective operations).
     */
    SerialCommandManager* getComputerSerial() const { return _computerSerial; }
};