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
#include "OtaManager.h"

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>

#include "SystemFunctions.h"
#include "ConfigManager.h"
#include "FirmwareVersion.h"
#include "SystemDefinitions.h"

// Buffer sizes — kept small to avoid large stack allocations.
static constexpr size_t OtaHashHexLen = 64;   // SHA-256 hex string
static constexpr size_t OtaDownloadBufSize = 512;  // streaming chunk size
static constexpr int OtaHttpTimeout = 15000; // ms
constexpr uint8_t MaxVersionSize = 18;

// ─── OtaManager ───────────────────────────────────────────────────────────────

OtaManager::OtaManager(IWifiController* wifiController, BroadcastManager* broadcaster)
    : _wifiController(wifiController)
    , _broadcaster(broadcaster)
    , _state(OtaState::Idle)
    , _lastCheckMs(0)
    , _nextUpdateMs(0)
    , _triggerCheck(false)
    , _triggerApply(false)
{
    _availableVersion[0] = '\0';
}

void OtaManager::begin()
{
    // Stagger the first automatic check so it doesn't fire immediately on boot —
    // give the system 60 s to fully connect before the first check.
    _lastCheckMs = SystemFunctions::millis64();

    if (_broadcaster)
        _broadcaster->sendDebug("OTA: auto-check enabled, 24h interval", "OTA");
}

void OtaManager::update(unsigned long now)
{
    // Only run at most once per second — avoids burning CPU on every loop() tick.
    if (now < _nextUpdateMs)
        return;

    _nextUpdateMs = now + OtaUpdatePollMs;

    // Only operate in WiFi client mode (not AP mode) and when connected.
    if (!isWifiClientMode())
        return;

    // Determine whether we should start a check this iteration.
    bool timeToCheck = (now - _lastCheckMs) >= OtaCheckIntervalMs;
    bool shouldCheck = _triggerCheck || timeToCheck;

    if (_state == OtaState::Idle && shouldCheck)
    {
        _lastCheckMs  = now;
        _triggerCheck = false;
        _state = OtaState::Checking;
        broadcastStatus();

        char tag[MaxVersionSize] = {};
        bool found = fetchLatestTag(tag, sizeof(tag));

        if (!found)
        {
            if (_broadcaster)
                _broadcaster->sendError("OTA: failed to fetch latest release tag", "OTA");

            _state = OtaState::Failed;
            broadcastStatus();
            _state = OtaState::Idle;
            return;
        }

        if (!isNewerVersion(tag))
        {
            if (_broadcaster)
            {
                char msg[48];
                snprintf(msg, sizeof(msg), "OTA: up to date (v%u.%u.%u.%u)",
                    FirmwareMajor, FirmwareMinor, FirmwarePatch, FirmwareBuild);
                _broadcaster->sendDebug(msg, "OTA");
            }

            _state = OtaState::UpToDate;
            broadcastStatus();
            _state = OtaState::Idle;
            return;
        }

        strncpy(_availableVersion, tag, sizeof(_availableVersion) - 1);
        _availableVersion[sizeof(_availableVersion) - 1] = '\0';

        if (_broadcaster)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: update available %s", tag);
            _broadcaster->sendDebug(msg, "OTA");
        }

        _state = OtaState::UpdateAvailable;
        broadcastStatus();

        // Check auto-apply flag from persistent header.
        SystemHeader* hdr = ConfigManager::getHeaderPtr();
        bool autoApply = hdr && (hdr->reserved[0] & OtaFlagAutoApply);

        if (!_triggerApply && !autoApply)
        {
            // Notify caller that an update is ready; they can call triggerCheck(true) to apply.
            return;
        }

        // Fall through to the apply block below in the same update() call.
        _triggerApply = true;
    }

    if (_state == OtaState::UpdateAvailable && (_triggerApply))
    {
        _triggerApply = false;
        _state = OtaState::Downloading;
        broadcastStatus();

        char tag[MaxVersionSize];
        strncpy(tag, _availableVersion, sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';

        char expectedHash[OtaHashHexLen + 1] = {};
        bool hashOk = fetchChecksum(tag, expectedHash, sizeof(expectedHash));

        if (!hashOk)
        {
            if (_broadcaster)
                _broadcaster->sendError("OTA: failed to fetch checksum", "OTA");

            _state = OtaState::Failed;
            broadcastStatus();
            _state = OtaState::Idle;
            return;
        }

        if (_broadcaster)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: downloading %s", tag);
            _broadcaster->sendDebug(msg, "OTA");
        }

        bool applied = downloadAndApply(tag, expectedHash);

        if (!applied)
        {
            if (_broadcaster)
                _broadcaster->sendError("OTA: download or apply failed", "OTA");

            _state = OtaState::Failed;
            broadcastStatus();
            _state = OtaState::Idle;
            return;
        }

        if (_broadcaster)
            _broadcaster->sendDebug("OTA: rebooting to complete update", "OTA");

        _state = OtaState::Rebooting;
        broadcastStatus();
        delay(500);
        ESP.restart();
    }
}

void OtaManager::triggerCheck(bool applyIfAvailable)
{
    _triggerCheck = true;
    _triggerApply = applyIfAvailable;
    _nextUpdateMs = 0;  // run on next update() call rather than waiting up to 1 s

    // If an update is already sitting in UpdateAvailable state and the caller
    // wants to apply it, jump straight to the apply phase on next update().
    if (applyIfAvailable && _state == OtaState::UpdateAvailable)
        _triggerApply = true;
}

// ─── private helpers ──────────────────────────────────────────────────────────

bool OtaManager::isWifiClientMode() const
{
    if (!_wifiController || !_wifiController->isEnabled())
        return false;

    return _wifiController->getConnectionState() == WifiConnectionState::Connected;
}

bool OtaManager::fetchLatestTag(char* tagOut, size_t tagLen)
{
    char url[128];
    snprintf(url, sizeof(url),
        "https://api.github.com/repos/%s/%s/releases/latest",
        OtaGithubOwner, OtaGithubRepo);

    WiFiClientSecure secureClient;
    secureClient.setCACert(OtaGithubRootCA);

    HTTPClient http;
    http.begin(secureClient, url);
    http.setTimeout(OtaHttpTimeout);
    http.addHeader("User-Agent", "SmartFuseBox-OTA");
    http.addHeader("Accept", "application/vnd.github+json");

    if (_broadcaster)
        _broadcaster->sendDebug("OTA: querying GitHub releases API", "OTA");

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        if (_broadcaster)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: API request failed, HTTP %d", code);
            _broadcaster->sendError(msg, "OTA");
        }
        http.end();
        return false;
    }

    // html_url appears within the first ~200 bytes of the response, e.g.:
    //   "html_url": "https://github.com/.../releases/tag/v0.9.2.2"
    // Read until we have captured the closing quote of that value — far
    // smaller than waiting for tag_name which is buried ~800 bytes in.
    // Read directly from secureClient — it IS the transport after http.GET().
    static constexpr size_t TagJsonBufSize = 512;
    char body[TagJsonBufSize] = {};
    size_t bodyLen = 0;

    while (secureClient.connected() && bodyLen < TagJsonBufSize - 1)
    {
        if (!secureClient.available())
        {
            delay(1);
            continue;
        }

        body[bodyLen++] = static_cast<char>(secureClient.read());

        // Stop as soon as the html_url value is fully received.
        if (bodyLen > 11)
        {
            const char* key = "\"html_url\":\"";
            const char* found = strstr(body, key);
            if (found)
            {
                const char* val = found + strlen(key);
                const char* end = strchr(val, '"');
                if (end)
                    break;
            }
        }
    }

    http.end();

    if (_broadcaster)
    {
        char msg[48];
        snprintf(msg, sizeof(msg), "OTA: read %u bytes from API", (unsigned)bodyLen);
        _broadcaster->sendDebug(msg, "OTA");

        if (strstr(body, "\"html_url\":\""))
            _broadcaster->sendDebug("OTA: html_url found in buffer", "OTA");
        else
            _broadcaster->sendError("OTA: html_url not found in buffer", "OTA");
    }

    bool ok = parseTagName(body, bodyLen, tagOut, tagLen);

    if (_broadcaster)
    {
        if (ok)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: tag extracted: %s", tagOut);
            _broadcaster->sendDebug(msg, "OTA");
        }
        else
        {
            _broadcaster->sendError("OTA: failed to extract tag from html_url", "OTA");
        }
    }

    return ok;
}

bool OtaManager::parseTagName(const char* json, size_t jsonLen, char* tagOut, size_t tagLen)
{
    // Locate the html_url value, e.g.:
    //   "html_url": "https://github.com/.../releases/tag/v0.9.2.2"
    // The version tag is the final path segment — everything after the last /
    // and before the closing double-quote.
    const char* key = "\"html_url\":";
    const char* p   = nullptr;

    for (size_t i = 0; i + strlen(key) < jsonLen; ++i)
    {
        if (strncmp(json + i, key, strlen(key)) == 0)
        {
            p = json + i + strlen(key);
            break;
        }
    }

    if (!p)
        return false;

    // Find the closing quote of the URL value.
    const char* endQuote = strchr(p, '"');
    if (!endQuote)
        return false;

    // Walk back from the closing quote to the last '/'.
    const char* lastSlash = endQuote - 1;
    while (lastSlash > p && *lastSlash != '/')
        --lastSlash;

    if (*lastSlash != '/')
        return false;

    // Copy the segment after the slash up to the closing quote.
    const char* start = lastSlash + 1;
    size_t len = static_cast<size_t>(endQuote - start);

    if (len == 0 || len >= tagLen)
        return false;

    strncpy(tagOut, start, len);
    tagOut[len] = '\0';

    return true;
}

bool OtaManager::isNewerVersion(const char* tag) const
{
    uint8_t maj = 0, min = 0, pat = 0, bld = 0;
    parseVersionTag(tag, maj, min, pat, bld);

    if (maj != FirmwareMajor)
        return maj > FirmwareMajor;

    if (min != FirmwareMinor)
        return min > FirmwareMinor;

    if (pat != FirmwarePatch)
        return pat > FirmwarePatch;

    return bld > FirmwareBuild;
}

void OtaManager::parseVersionTag(const char* tag, uint8_t& major, uint8_t& minor,
                                  uint8_t& patch, uint8_t& build) const
{
    // Expected format: "v<major>.<minor>.<patch>.<build>"  (leading 'v' is optional)
    const char* p = tag;

    if (*p == 'v' || *p == 'V')
        ++p;

    major = static_cast<uint8_t>(strtoul(p, const_cast<char**>(&p), 10)); if (*p == '.') ++p;
    minor = static_cast<uint8_t>(strtoul(p, const_cast<char**>(&p), 10)); if (*p == '.') ++p;
    patch = static_cast<uint8_t>(strtoul(p, const_cast<char**>(&p), 10)); if (*p == '.') ++p;
    build = static_cast<uint8_t>(strtoul(p, const_cast<char**>(&p), 10));
}

bool OtaManager::fetchChecksum(const char* tag, char* hashHexOut, size_t hashLen)
{
    char url[256];
    snprintf(url, sizeof(url),
        "https://github.com/%s/%s/releases/download/%s/SmartFuseBox-%s-%s.sha256",
        OtaGithubOwner, OtaGithubRepo, tag, CONFIG_IDF_TARGET, tag);

    WiFiClientSecure secureClient;
    secureClient.setCACert(OtaGithubRootCA);

    HTTPClient http;
    http.begin(secureClient, url);
    http.setTimeout(OtaHttpTimeout);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "SmartFuseBox-OTA");

    if (_broadcaster)
    {
        char msg[48];
        snprintf(msg, sizeof(msg), "OTA: fetching checksum for %s", tag);
        _broadcaster->sendDebug(msg, "OTA");
    }

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        if (_broadcaster)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: checksum request failed, HTTP %d", code);
            _broadcaster->sendError(msg, "OTA");
        }

        http.end();
        return false;
    }

    // checksum.sha256 format:  "<64-char-hex>  <filename>\n"  or just  "<64-char-hex>"
    // Read exactly OtaHashHexLen characters — that is all we need.
    // Read directly from secureClient — it IS the transport after http.GET().
    char body[OtaHashHexLen + 1] = {};
    size_t bodyLen = 0;

    while (secureClient.connected() && bodyLen < OtaHashHexLen)
    {
        if (!secureClient.available())
        {
            delay(1);
            continue;
        }
        body[bodyLen++] = static_cast<char>(secureClient.read());
    }

    body[bodyLen] = '\0';
    http.end();

    // Extract the first 64 hex characters (the hash itself).
    if (bodyLen < OtaHashHexLen)
    {
        if (_broadcaster)
            _broadcaster->sendError("OTA: checksum file invalid or too short", "OTA");
        return false;
    }

    strncpy(hashHexOut, body, OtaHashHexLen);
    hashHexOut[OtaHashHexLen] = '\0';

    // Validate that it really looks like a hex string.
    for (size_t i = 0; i < OtaHashHexLen; ++i)
    {
        char c = hashHexOut[i];

        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return false;
    }

    return true;
}

bool OtaManager::downloadAndApply(const char* tag, const char* expectedHash)
{
    char url[256];
    snprintf(url, sizeof(url),
        "https://github.com/%s/%s/releases/download/%s/SmartFuseBox-%s-%s.bin",
        OtaGithubOwner, OtaGithubRepo, tag, CONFIG_IDF_TARGET, tag);

    WiFiClientSecure secureClient;
    secureClient.setCACert(OtaGithubRootCA);

    HTTPClient http;
    http.begin(secureClient, url);
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "SmartFuseBox-OTA");

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        if (_broadcaster)
        {
            char msg[48];
            snprintf(msg, sizeof(msg), "OTA: binary request failed, HTTP %d", code);
            _broadcaster->sendError(msg, "OTA");
        }
        http.end();
        return false;
    }

    int contentLength = http.getSize();

    if (contentLength <= 0)
    {
        if (_broadcaster)
            _broadcaster->sendError("OTA: binary has no content length", "OTA");
        http.end();
        return false;
    }

    if (_broadcaster)
    {
        char msg[48];
        snprintf(msg, sizeof(msg), "OTA: binary size %d bytes", contentLength);
        _broadcaster->sendDebug(msg, "OTA");
    }

    if (!Update.begin(static_cast<size_t>(contentLength), U_FLASH))
    {
        if (_broadcaster)
            _broadcaster->sendError("OTA: Update.begin failed", "OTA");
        http.end();
        return false;
    }

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);  // 0 = SHA-256

    // Read directly from secureClient — it IS the transport after http.GET(),
    // so no getStreamPtr() / WiFiClient* needed.
    uint8_t buf[OtaDownloadBufSize];
    int remaining = contentLength;
    int lastProgressPct = 0;
    unsigned long deadline = SystemFunctions::millis64() + 120000UL;  // 2-minute download budget

    while (remaining > 0 && SystemFunctions::millis64() < deadline)
    {
        if (!secureClient.available())
        {
            delay(1);
            continue;
        }

        size_t toRead = (remaining < static_cast<int>(sizeof(buf)))
                      ? static_cast<size_t>(remaining)
                      : sizeof(buf);
        size_t got = secureClient.readBytes(buf, toRead);

        if (got == 0)
            continue;

        mbedtls_sha256_update(&sha, buf, got);

        if (Update.write(buf, got) != got)
        {
            if (_broadcaster)
                _broadcaster->sendError("OTA: flash write error", "OTA");
            Update.abort();
            mbedtls_sha256_free(&sha);
            http.end();
            return false;
        }

        remaining -= static_cast<int>(got);

        if (_broadcaster && contentLength > 0)
        {
            int pct = ((contentLength - remaining) * 100) / contentLength;
            int milestone = (pct / 25) * 25;
            if (milestone > lastProgressPct && milestone < 100)
            {
                lastProgressPct = milestone;
                char msg[32];
                snprintf(msg, sizeof(msg), "OTA: download %d%%", milestone);
                _broadcaster->sendDebug(msg, "OTA");
            }
        }
    }

    http.end();

    uint8_t hashBytes[32];
    mbedtls_sha256_finish(&sha, hashBytes);
    mbedtls_sha256_free(&sha);

    if (remaining != 0)
    {
        if (_broadcaster)
            _broadcaster->sendError("OTA: download timed out or incomplete", "OTA");
        Update.abort();
        return false;
    }

    if (_broadcaster)
        _broadcaster->sendDebug("OTA: verifying SHA-256 hash", "OTA");

    // Convert computed hash to lowercase hex for comparison.
    char computed[OtaHashHexLen + 1] = {};

    for (int i = 0; i < 32; ++i)
        snprintf(computed + i * 2, 3, "%02x", hashBytes[i]);

    // Case-insensitive comparison (checksum file may use uppercase).
    char expected[OtaHashHexLen + 1];
    strncpy(expected, expectedHash, sizeof(expected) - 1);
    expected[sizeof(expected) - 1] = '\0';

    for (size_t i = 0; i < OtaHashHexLen; ++i)
        expected[i] = static_cast<char>(tolower(static_cast<unsigned char>(expected[i])));

    if (strncmp(computed, expected, OtaHashHexLen) != 0)
    {
        if (_broadcaster)
            _broadcaster->sendError("OTA: hash mismatch, aborting", "OTA");
        Update.abort();
        return false;
    }

    if (_broadcaster)
        _broadcaster->sendDebug("OTA: hash verified, applying update", "OTA");

    return Update.end(true);
}

void OtaManager::broadcastStatus()
{
    if (!_broadcaster)
        return;

    // Build current firmware version string once.
    char current[MaxVersionSize];
    snprintf(current, sizeof(current), "v%u.%u.%u.%u",
        FirmwareMajor, FirmwareMinor, FirmwarePatch, FirmwareBuild);

    const char* stateStr = "idle";

    switch (_state)
    {
        case OtaState::Checking:
            stateStr = "checking";
            break;
        case OtaState::UpdateAvailable:
            stateStr = "available";
            break;
        case OtaState::Downloading:
            stateStr = "downloading";
            break;
        case OtaState::Rebooting:
            stateStr = "rebooting";
            break;
        case OtaState::Failed:
            stateStr = "failed";
            break;
        case OtaState::UpToDate:
            stateStr = "uptodate";
            break;
        default:
            stateStr = "idle";
            break;
    }

    char payload[64] = {};
    snprintf(payload, sizeof(payload), "v=%s;av=%s;s=%s",
        current, _availableVersion, stateStr);

    _broadcaster->sendCommand(SystemCheckForUpdate, payload);
}

#endif // OTA_AUTO_UPDATE && ESP32 && WIFI_SUPPORT
