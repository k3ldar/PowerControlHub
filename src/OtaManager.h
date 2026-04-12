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
#include "SystemFunctions.h"

// How often to poll GitHub for a new release when no manual trigger is active.
constexpr uint64_t OtaCheckIntervalMs = 24UL * 60UL * 60UL * 1000UL;

// How often update() checks whether a timed or triggered action is due.
// Keeps the method near-zero cost between checks.
constexpr uint64_t OtaUpdatePollMs = 10000UL;

// GitHub repository owner and name — update if the repo is ever moved.
constexpr char OtaGithubOwner[] = "SmartFuseBox";
constexpr char OtaGithubRepo[]  = "SmartFuseBox";

// Asset filenames include both the board target and the version tag, e.g.:
//   SmartFuseBox-esp32-v0.9.2.2.bin
//   SmartFuseBox-esp32-v0.9.2.2.sha256
// They are constructed at runtime in fetchChecksum() and downloadAndApply()
// using CONFIG_IDF_TARGET (set by the ESP32 core) and the fetched tag string.

// Root CA certificate for api.github.com and github.com (release download redirect).
// Both domains chain to "Sectigo Public Server Authentication Root E46".
// Thumbprint: EC8A396C40F02EBC4275D49FAB1C1A5B67BED29A  — valid until 2046-03-21.
static const char OtaGithubRootCA[] PROGMEM = R"(
-----BEGIN CERTIFICATE-----
MIICOjCCAcGgAwIBAgIQQvLM2htpN0RfFf51KBC49DAKBggqhkjOPQQDAzBfMQsw
CQYDVQQGEwJHQjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1T
ZWN0aWdvIFB1YmxpYyBTZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwHhcN
MjEwMzIyMDAwMDAwWhcNNDYwMzIxMjM1OTU5WjBfMQswCQYDVQQGEwJHQjEYMBYG
A1UEChMPU2VjdGlnbyBMaW1pdGVkMTYwNAYDVQQDEy1TZWN0aWdvIFB1YmxpYyBT
ZXJ2ZXIgQXV0aGVudGljYXRpb24gUm9vdCBFNDYwdjAQBgcqhkjOPQIBBgUrgQQA
IgNiAAR2+pmpbiDt+dd34wc7qNs9Xzjoq1WmVk/WSOrsfy2qw7LFeeyZYX8QeccC
WvkEN/U0NSt3zn8gj1KjAIns1aeibVvjS5KToID1AZTc8GgHHs3u/iVStSBDHBv+
6xnOQ6OjQjBAMB0GA1UdDgQWBBTRItpMWfFLXyY4qp3W7usNw/upYTAOBgNVHQ8B
Af8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNnADBkAjAn7qRa
qCG76UeXlImldCBteU/IvZNeWBj7LRoAasm4PdCkT0RHlAFWovgzJQxC36oCMB3q
4S6ILuH5px0CMk7yn2xVdOOurvulGu7t0vzCAxHrRVxgED1cf5kDW21USAGKcw==
-----END CERTIFICATE-----
)";

// Root CA certificate for release-assets.githubusercontent.com (GitHub CDN).
// The CDN presents a Let's Encrypt certificate chaining to "ISRG Root X1".
// Thumbprint: CABD2A79A1076A31F21D253635CB039D4329A5E8  — valid until 2035-06-04.
// Used by setCACert() for the second (CDN) hop in fetchChecksum/downloadAndApply.
static const char OtaCdnRootCA[] PROGMEM = R"(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPAm
RGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)";

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
    uint64_t _lastCheckMs;
    uint64_t _nextUpdateMs;
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
    void update(uint64_t now);

    // Request an immediate version check. Pass applyIfAvailable = true to also
    // apply the update without waiting for the auto-apply flag (one-shot manual update).
    void triggerCheck(bool applyIfAvailable = false);

    OtaState getState() const { return _state; }
    const char* getAvailableVersion() const { return _availableVersion; }
    bool isUpdateAvailable() const { return _state == OtaState::UpdateAvailable; }
};

#endif // OTA_AUTO_UPDATE && ESP32 && WIFI_SUPPORT
