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

#include "Local.h"
#include "NavigationController.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include "NextionIds.h"

// ---------------------------------------------------------------------------
// Page order for the main next/previous chain.
// Maritime-only pages are included here; buildPageList() filters them out
// when the LocationType is not maritime.
// ---------------------------------------------------------------------------
static const uint8_t s_allPages[] =
{
    PageIdHome,
    PageIdWarning,
    PageIdRelay,
    PageIdVhfRadio,           // maritime
    PageIdSoundSignals,       // maritime
    PageIdFlags,              // maritime
    PageIdCardinalMarkers,    // maritime
    PageIdBuoys,              // maritime
    PageIdMoonPhases,         // maritime
    PageIdVhfDistress,        // maritime (reachable from VhfRadio button, not prev/next)
    PageIdVhfChannels,        // maritime (reachable from VhfRadio button, not prev/next)
    PageIdSystem,
    PageIdEnvironment,
    PageIdAbout,
    PageIdSettingsRelays,
    PageIdSettings,
};

// Pages that are only shown for maritime LocationTypes.
static const uint8_t s_maritimePages[] =
{
    PageIdVhfRadio,
    PageIdSoundSignals,
    PageIdFlags,
    PageIdCardinalMarkers,
    PageIdBuoys,
    PageIdMoonPhases,
    PageIdVhfDistress,
    PageIdVhfChannels,
};

// ---------------------------------------------------------------------------

NavigationController::NavigationController(NextionControl* nextion, LocationType locationType)
    : _nextion(nextion)
{
    buildPageList(locationType);
}

// INavigationDelegate ---------------------------------------------------------

void NavigationController::navigateNext(uint8_t fromPageId)
{
    if (_activePages.empty() || _nextion == nullptr)
        return;

    size_t idx = indexOf(fromPageId);

    if (idx == SIZE_MAX)
    {
        // Current page not in the active list — go to the first active page.
        setPage(_activePages[0]);
        return;
    }

    size_t next = (idx + 1) % _activePages.size();
    setPage(_activePages[next]);
}

void NavigationController::navigatePrevious(uint8_t fromPageId)
{
    if (_activePages.empty() || _nextion == nullptr)
        return;

    size_t idx = indexOf(fromPageId);

    if (idx == SIZE_MAX)
    {
        setPage(_activePages[0]);
        return;
    }

    size_t prev = (idx == 0) ? _activePages.size() - 1 : idx - 1;
    setPage(_activePages[prev]);
}

void NavigationController::navigateTo(uint8_t targetPageId)
{
    if (_nextion == nullptr)
        return;

    // Silently ignore navigation to pages that are not active.
    if (!isPageActive(targetPageId))
        return;

    setPage(targetPageId);
}

// Public -----------------------------------------------------------------------

void NavigationController::rebuildForLocationType(LocationType locationType)
{
    buildPageList(locationType);
}

bool NavigationController::isPageActive(uint8_t pageId) const
{
    return indexOf(pageId) != SIZE_MAX;
}

// Private ---------------------------------------------------------------------

void NavigationController::buildPageList(LocationType locationType)
{
    _activePages.clear();
    bool maritime = isMaritime(locationType);

    for (uint8_t id : s_allPages)
    {
        if (!maritime)
        {
            // Check if this page is maritime-only.
            bool skip = false;
            for (uint8_t m : s_maritimePages)
            {
                if (m == id) { skip = true; break; }
            }
            if (skip)
                continue;
        }
        _activePages.push_back(id);
    }
}

size_t NavigationController::indexOf(uint8_t pageId) const
{
    for (size_t i = 0; i < _activePages.size(); ++i)
    {
        if (_activePages[i] == pageId)
            return i;
    }
    return SIZE_MAX;
}

bool NavigationController::isMaritime(LocationType lt)
{
    return lt == LocationType::Power  ||
           lt == LocationType::Sail   ||
           lt == LocationType::Fishing ||
           lt == LocationType::Yacht;
}

void NavigationController::setPage(uint8_t pageId) const
{
    if (_nextion)
    {
        char cmd[16];
        snprintf(cmd, sizeof(cmd), "page %d", pageId);
        _nextion->sendCommand(cmd);
    }
}

#endif // NEXTION_DISPLAY_DEVICE
