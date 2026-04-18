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

#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include <Arduino.h>
#include <SerialCommandManager.h>
#include "ConfigController.h"
#include "RelayController.h"
#include "SdCardLogger.h"

constexpr char SD_CONFIG_FILENAME[] = "config.txt";
constexpr uint16_t SD_CONFIG_MAX_LINE_LENGTH = 128;

/**
 * @class SdCardConfigLoader
 * @brief Loads configuration from SD card and applies to ConfigManager
 * 
 * Boot sequence:
 * 1. Check for SD card with config.txt
 * 2. Parse and validate all commands
 * 3. Compare with current EEPROM config
 * 4. If different, apply changes and save to EEPROM
 * 5. Send config via LINK to sync control panel
 * 
 * Features:
 * - Read-only SD card config (not auto-updated during runtime)
 * - Command format validation
 * - Error logging to Serial
 * - C29: Reload config from SD card
 * - C30: Export current config to SD card
 * 
 * Usage:
 * @code
 * SdCardConfigLoader loader(&commandMgrComputer, &configController);
 * 
 * void setup()
 * {
 *     bool sdConfigLoaded = loader.loadConfigFromSd();
 *     if (sdConfigLoaded)
 *     {
 *         // SD config was applied
 *     }
 * }
 * @endcode
 */
class SdCardConfigLoader
{
private:
    SerialCommandManager* _computerSerial;
    ConfigController* _configController;
    RelayController* _relayController;
    bool _sdConfigPresent;

    /**
     * @brief Check if SD card is initialized and ready for operations
     * @return true if MicroSdDriver is ready (initialized, present, not in exclusive mode)
     */
    bool checkSdCard();

    /**
     * @brief Check if config.txt exists on SD card
     * @return true if config file exists
     */
    bool configFileExists();

    /**
     * @brief Parse and apply a single config command line
     * @param line Command line to parse
     * @return true if command was successfully applied
     */
    bool applyConfigCommand(const char* line);

    /**
     * @brief Log error message to serial
     * @param message Error message
     * @param line Optional line content that caused error
     */
    void logError(const char* message, const char* line = nullptr);

    /**
     * @brief Log info message to serial
     * @param message Info message
     */
    void logInfo(const char* message);

public:
    /**
     * @brief Constructor
     * @param computerSerial Serial manager for computer communication
     * @param linkSerial Serial manager for LINK communication
     * @param configController Configuration controller
     */
    SdCardConfigLoader(SerialCommandManager* computerSerial,
                       ConfigController* configController,
                       RelayController* relayController);

    /**
     * @brief Load configuration from SD card if present
     * 
     * Reads config.txt, applies all commands, saves to EEPROM if changed,
     * and syncs to control panel via LINK.
     * 
     * @return true if SD config was found and applied
     */
    bool loadConfigFromSd();

    /**
     * @brief Reload configuration from SD card (C29 command)
     * @return true if config was reloaded successfully
     */
    bool reloadConfigFromSd();

    /**
     * @brief Export current configuration to SD card (C30 command)
     * @return true if config was exported successfully
     */
    bool exportConfigToSd();

    /**
     * @brief Check if SD config was loaded at boot
     * @return true if SD config is present and was loaded
     */
    bool isSdConfigPresent() const { return _sdConfigPresent; }

    /**
     * @brief Callback handler for SD card ready events
     * 
     * Called automatically when:
     * - SD card completes initialization
     * - A different card is inserted
     * 
     * Attempts to load config from the newly available card.
     * 
     * @param isNewCard True if this is a new card (swap), false for initial ready
     */
    void onSdCardReady(bool isNewCard);
};

#endif
