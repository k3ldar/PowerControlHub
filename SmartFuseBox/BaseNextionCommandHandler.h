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

#if defined(NEXTION_DISPLAY_DEVICE)
#include <NextionControl.h>
#endif

#include "SharedBaseCommandHandler.h"
#include "WarningManager.h"

/**
 * @brief Base class for command handlers that interact with boat-specific systems.
 * 
 * This class extends SharedBaseCommandHandler with additional boat-specific dependencies
 * and helper methods used by command handlers that need to:
 * - Send commands, debug messages, and errors via BroadcastManager (inherited)
 * - Notify the current Nextion display page of updates
 * - Access the warning management system
 * - Parse common data formats (booleans, digits)
 * 
 * Handlers like AckCommandHandler, SensorCommandHandler, and WarningCommandHandler
 * should inherit from this class to get access to these shared capabilities.
 * 
 * For handlers that only need broadcasting (like some shared command handlers),
 * inherit from SharedBaseCommandHandler instead.
 * For handlers that don't need any of these dependencies (like ConfigCommandHandler),
 * inherit directly from BaseCommandHandler.
 */
class BaseNextionCommandHandler : public SharedBaseCommandHandler
{
protected:
    /**
     * @brief Constructor with boat-specific dependencies.
     * 
     * @param broadcaster Manager for broadcasting commands/debug/errors
     * @param nextionControl Controller for interacting with Nextion display
     * @param warningManager Manager for system warnings (can be nullptr if not needed)
     */
    BaseNextionCommandHandler(
        BroadcastManager* broadcaster,
#if defined(NEXTION_DISPLAY_DEVICE)
        NextionControl* nextionControl,
#endif
        WarningManager* warningManager = nullptr
    );

    /**
     * @brief Notify the current display page of an external update.
     * 
     * This is a convenience wrapper that safely gets the current page from
     * NextionControl and calls handleExternalUpdate on it.
     * 
     * @param updateType Type of update (cast to uint8_t from PageUpdateType enum)
     * @param data Optional pointer to update-specific data structure
     */
    void notifyCurrentPage(uint8_t updateType, const void* data)
    {
#if defined(NEXTION_DISPLAY_DEVICE)
        if (!_nextionControl)
            return;

        BaseDisplayPage* p = _nextionControl->getCurrentPage();

        if (!p)
            return;

        p->handleExternalUpdate(updateType, data);
#endif
    }

#if defined(NEXTION_DISPLAY_DEVICE)
    // Protected member variables for derived classes to access
    // Note: _broadcaster is inherited from SharedBaseCommandHandler
    NextionControl* _nextionControl;
#endif
};
