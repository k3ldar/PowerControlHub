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

#if defined(NEXTION_DISPLAY_DEVICE)

#include <Arduino.h>
#include <BaseDisplayPage.h>
#include <SerialCommandManager.h>
#include "Config.h"
#include "INavigationDelegate.h"
#include "WarningManager.h"
#include "NextionIds.h"

// Update type constants for external updates
enum class PageUpdateType : uint8_t
{
    None = 0x00,
    Warning = 0x01,
    RelayState = 0x02,
    HeartbeatAck = 0x03,
    Temperature = 0x04,
    Humidity = 0x05,
    Bearing = 0x06,
    Direction = 0x07,
    Speed = 0x08,
    CompassTemp = 0x09,
    WaterLevel = 0x0A,
    WaterPumpActive = 0x0B,
    SoundSignal = 0x0C,
	CpuUsage = 0x0D,
	MemoryUsage = 0x0E,
	GpsLatitude = 0x0F,
	GpsLongitude = 0x10,
	GpsSatellites = 0x11,
	GpsAltitude = 0x12,
    GpsDistance = 0x13,
	Daytime = 0x14,
	HornActive = 0x15
};

// Data structure for relay state updates
struct RelayStateUpdate {
    uint8_t relayIndex;  // 0-based relay index (0..7)
    bool isOn;           // true = relay on, false = relay off
};

struct FloatStateUpdate {
    float value;
};

struct DoubleStateUpdate {
    double value;
};

struct UInt8Update {
    uint8_t value;
};

struct UInt16Update {
    uint16_t value;
};

struct BoolStateUpdate {
    bool value;
};

// Data structure for string/text updates (e.g., direction like "NNW", "SE")
struct CharStateUpdate {
    static const uint8_t MaxLength = 16; // Sufficient for compass directions, status strings, etc.
    uint8_t length;           // Actual length of the string (not including null terminator)
    char value[MaxLength];   // Fixed-size buffer for the string (null-terminated)
};

/**
 * @class BasePage
 * @brief Intermediate base class for all nextion display pages.
 * 
 * Provides common functionality and shared state for all pages in the boat
 * control panel application, including warning state management, configuration
 * access, and other shared behaviors.
 */
class BasePage : public BaseDisplayPage
{
private:
    // Shared configuration pointer
    Config* _config;

    // Command managers shared across all pages
    SerialCommandManager* _commandMgrComputer;

    // Warning manager (shared across all pages)
    WarningManager* _warningManager;

    // Navigation delegate — set by NextionFactory after construction.
    INavigationDelegate* _navDelegate = nullptr;

protected:
    
    /**
     * @brief Constructor for boat pages.
     * @param serialPort Pointer to the Nextion serial stream
     * @param warningMgr Pointer to the shared WarningManager
     * @param commandMgrComputer Pointer to the SerialCommandManager for computer communication (optional)
     */
    explicit BasePage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer = nullptr);
    
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~BasePage();
    
    /**
     * @brief Get the configuration pointer.
     * @return Pointer to the configuration, or nullptr if not set
     */
    Config* getConfig() const { return _config; }

    /**
     * @brief Get the computer command manager.
     * @return Pointer to the computer SerialCommandManager, or nullptr if not set
     */
    SerialCommandManager* getCommandMgrComputer() const { return _commandMgrComputer; }

    /**
     * @brief Get the warning manager.
     * @return Pointer to the WarningManager
     */
    WarningManager* getWarningManager() const { return _warningManager; }

    /**
     * @brief Fire a "go to next page" navigation event.
     * Safe to call even if no delegate has been set.
     */
    void navigateNext()     { if (_navDelegate) _navDelegate->navigateNext(getPageId()); }

    /**
     * @brief Fire a "go to previous page" navigation event.
     * Safe to call even if no delegate has been set.
     */
    void navigatePrevious() { if (_navDelegate) _navDelegate->navigatePrevious(getPageId()); }

    /**
     * @brief Fire a "go to specific page" navigation event.
     * The delegate silently ignores the call if the target page is not active.
     * @param targetPageId Nextion page ID to navigate to.
     */
    void navigateTo(uint8_t targetPageId) { if (_navDelegate) _navDelegate->navigateTo(targetPageId); }

    /**
	* @brief Get the appropriate button color based on state.
	* @return Color index for the button
     */
    uint8_t getButtonColor(uint8_t buttonIndex, bool isOn, uint8_t maxButtons);

public:
    
    /**
     * @brief Set the configuration pointer for this page.
     * @param config Pointer to the global configuration
     */
    void configSet(Config* config);

    /**
     * @brief Called when configuration has been updated.
     * 
     * Override in derived classes to respond to configuration changes.
     * Default implementation does nothing. This is public so external
     * command handlers can notify pages when config changes.
     */
    virtual void configUpdated();

    /**
     * @brief Inject the navigation delegate.
     * Called by NextionFactory immediately after page construction.
     */
    void setNavigationDelegate(INavigationDelegate* delegate) { _navDelegate = delegate; }
};

#endif // NEXTION_DISPLAY_DEVICE
