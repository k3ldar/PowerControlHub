/*
 * SmartFuseBox
 * Copyright (C) 2026 Simon Carter (s1cart3r@gmail.com)
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

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)

#include <Arduino.h>
#include "BroadcastManager.h"
#include "IWifiController.h"

// How often to poll GitHub for a new release when no manual trigger is active.
constexpr unsigned long OtaCheckIntervalMs = 24UL * 60UL * 60UL * 1000UL;

// How often update() checks whether a timed or triggered action is due.
// Keeps the method near-zero cost between checks.
constexpr unsigned long OtaUpdatePollMs = 100000UL;

// GitHub repository owner and name — update if the repo is ever moved.
constexpr char OtaGithubOwner[] = "SmartFuseBox";
constexpr char OtaGithubRepo[]  = "SmartFuseBox";

// Asset filenames include both the board target and the version tag, e.g.:
//   SmartFuseBox-esp32-v0.9.2.2.bin
//   SmartFuseBox-esp32-v0.9.2.2.sha256
// They are constructed at runtime in fetchChecksum() and downloadAndApply()
// using CONFIG_IDF_TARGET (set by the ESP32 core) and the fetched tag string.

enum class OtaState : uint8_t
{
    Idle = 0,  // waiting for next check interval or manual trigger
    Checking = 1,  // querying GitHub API for latest release
    UpdateAvailable = 2,  // newer version found; waiting for auto-apply or manual apply
    Downloading = 3,  // streaming binary and computing SHA-256
    Rebooting = 4,  // Update.end() succeeded; ESP.restart() imminent
    Failed = 5,  // last operation failed; returns to Idle after broadcast
    UpToDate = 6,  // already on latest; returns to Idle after broadcast
};

class OtaManager
{
private:
    IWifiController* _wifiController;
    BroadcastManager* _broadcaster;
    OtaState _state;
    unsigned long _lastCheckMs;
    unsigned long _nextUpdateMs;
    char _availableVersion[16];
    bool _triggerCheck;
    bool _triggerApply;

    bool isWifiClientMode() const;
    bool fetchLatestTag(char* tagOut, size_t tagLen);
    bool parseTagName(const char* json, size_t jsonLen, char* tagOut, size_t tagLen);
    bool isNewerVersion(const char* tag) const;
    void parseVersionTag(const char* tag, uint8_t& major, uint8_t& minor,
                         uint8_t& patch, uint8_t& build) const;
    bool fetchChecksum(const char* tag, char* hashHexOut, size_t hashLen);
    bool downloadAndApply(const char* tag, const char* expectedHash);
    void broadcastStatus();

public:
    OtaManager(IWifiController* wifiController, BroadcastManager* broadcaster);

    // Call once from SmartFuseBoxApp::setup().
    void begin();

    // Call every iteration of SmartFuseBoxApp::loop().
    void update(unsigned long now);

    // Request an immediate version check. Pass applyIfAvailable = true to also
    // apply the update without waiting for the auto-apply flag (one-shot manual update).
    void triggerCheck(bool applyIfAvailable = false);

    OtaState getState() const { return _state; }
    const char* getAvailableVersion() const { return _availableVersion; }
    bool isUpdateAvailable() const { return _state == OtaState::UpdateAvailable; }
};

#endif // OTA_AUTO_UPDATE && ESP32 && WIFI_SUPPORT
