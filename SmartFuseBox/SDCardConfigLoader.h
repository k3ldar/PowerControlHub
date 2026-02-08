#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <SerialCommandManager.h>
#include "ConfigController.h"
#include "ConfigSyncManager.h"
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
 * - Integration with ConfigSyncManager (disable if SD config loaded)
 * - C29: Reload config from SD card
 * - C30: Export current config to SD card
 * 
 * Usage:
 * @code
 * SdCardConfigLoader loader(&commandMgrComputer, &commandMgrLink, 
 *                           &configController, &configSyncManager, csPin);
 * 
 * void setup() {
 *     bool sdConfigLoaded = loader.loadConfigFromSd();
 *     if (sdConfigLoaded) {
 *         // SD config was applied, ConfigSyncManager will be disabled
 *     }
 * }
 * @endcode
 */
class SdCardConfigLoader
{
private:
    SerialCommandManager* _computerSerial;
    SerialCommandManager* _linkSerial;
    ConfigController* _configController;
    ConfigSyncManager* _configSyncManager;
    SdCardLogger* _sdCardLogger;
    uint8_t _csPin;
    bool _sdConfigPresent;

    // SD card
    SdFat _sd;

    /**
     * @brief Check if SD card is accessible
     * @return true if SD card is present and readable
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
     * @brief Send config to LINK serial to sync control panel
     */
    void syncConfigToLink();

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
     * @param configSyncManager Configuration sync manager (will be disabled if SD config loaded)
     * @param csPin Chip select pin for SD card
     */
    SdCardConfigLoader(SerialCommandManager* computerSerial,
                       SerialCommandManager* linkSerial,
                       ConfigController* configController,
                       ConfigSyncManager* configSyncManager,
                       uint8_t csPin);

    /**
     * @brief Set the SdCardLogger reference for coordinated SD card access
     * @param sdCardLogger Pointer to the SdCardLogger instance
     */
    void setSdCardLogger(SdCardLogger* sdCardLogger);

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
};
