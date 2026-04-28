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
#include <vector>
#include <NextionControl.h>

#include "INavigationDelegate.h"
#include "Config.h"

/**
 * @brief Centrally manages page navigation for the Nextion display.
 *
 * The controller holds the ordered list of page IDs that are active for the
 * current LocationType.  Pages fire events (next / previous / go-to) through
 * INavigationDelegate; the controller resolves the correct target and calls
 * NextionControl::setPage().  Pages never need to know their neighbours or
 * whether a peer page exists for the current location.
 *
 * Maritime pages (Buoys, CardinalMarkers, Flags, Sound*, Vhf*) are only
 * included in the active list when the LocationType is one of:
 *   Power, Sail, Fishing, Yacht
 */
class NavigationController : public INavigationDelegate
{
private:
    NextionControl* _nextion;
    std::vector<uint8_t> _activePages; // ordered list of active page IDs

    void buildPageList(LocationType locationType);

    // Returns the index of pageId in _activePages, or SIZE_MAX if not found.
    size_t indexOf(uint8_t pageId) const;

    static bool isMaritime(LocationType lt);

    void setPage(uint8_t pageId) const;

public:
    /**
     * @brief Construct the controller and build the initial page list.
     *
     * @param nextion     Pointer to the NextionControl instance (non-owning).
     * @param locationType The current vessel/location type from Config.
     */
    explicit NavigationController(NextionControl* nextion, LocationType locationType);

    // INavigationDelegate
    void navigateNext(uint8_t fromPageId) override;
    void navigatePrevious(uint8_t fromPageId) override;
    void navigateTo(uint8_t targetPageId) override;

    /**
     * @brief Rebuild the active page list after a LocationType change.
     *
     * Call this whenever the user changes the location type in settings so
     * that the page list is updated without a reboot.
     *
     * @param locationType The new location type.
     */
    void rebuildForLocationType(LocationType locationType);

    /**
     * @brief Check whether a page ID is present in the active list.
     * @return true if the page is active for the current LocationType.
     */
    bool isPageActive(uint8_t pageId) const;

};

#endif // NEXTION_DISPLAY_DEVICE
