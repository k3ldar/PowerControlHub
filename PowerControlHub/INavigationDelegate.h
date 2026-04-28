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

#include <stdint.h>

/**
 * @brief Pure interface that pages use to request navigation.
 *
 * Pages fire navigation events through this interface rather than calling
 * setPage() with hard-coded page IDs. The NavigationController implements
 * this interface and resolves the correct target, skipping pages that are
 * not active for the current LocationType.
 */
class INavigationDelegate
{
public:
    virtual ~INavigationDelegate() = default;

    /**
     * @brief Navigate to the next page in the active page list.
     * @param fromPageId The ID of the page firing the event.
     */
    virtual void navigateNext(uint8_t fromPageId) = 0;

    /**
     * @brief Navigate to the previous page in the active page list.
     * @param fromPageId The ID of the page firing the event.
     */
    virtual void navigatePrevious(uint8_t fromPageId) = 0;

    /**
     * @brief Navigate directly to a specific page.
     *
     * If the requested page is not active (filtered out by LocationType) the
     * call is silently ignored — pages never crash because a peer is absent.
     *
     * @param targetPageId The Nextion page ID to navigate to.
     */
    virtual void navigateTo(uint8_t targetPageId) = 0;
};

#endif // NEXTION_DISPLAY_DEVICE
