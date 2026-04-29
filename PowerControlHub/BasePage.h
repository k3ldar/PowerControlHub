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

#include "Local.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include <Arduino.h>
#include <BaseDisplayPage.h>
#include <NextionControl.h>
#include <SerialCommandManager.h>
#include "Config.h"
#include "INavigationDelegate.h"
#include "WarningManager.h"
#include "NextionIds.h"
#include "MessageBus.h"

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

    // MessageBus for event subscriptions
    MessageBus* _messageBus;

    // Navigation delegate — set by NextionFactory after construction.
    INavigationDelegate* _navDelegate = nullptr;

protected:

    explicit BasePage(Stream* serialPort,
        WarningManager* warningMgr,
        SerialCommandManager* commandMgrComputer = nullptr,
        MessageBus* messageBus = nullptr);

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
     * @brief Get the message bus.
     * @return Pointer to the MessageBus, or nullptr if not set
     */
    MessageBus* getMessageBus() const { return _messageBus; }

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
