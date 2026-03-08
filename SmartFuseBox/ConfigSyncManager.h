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
#include <SerialCommandManager.h>
#include "ConfigController.h"
#include "Config.h"
#include "WarningManager.h"

constexpr unsigned long DefaultConfigSyncIntervalMs = 300000; // 5 minutes (normal)
constexpr unsigned long FastConfigSyncRetryMs = 10000;        // 10 seconds (retry on failure)

// ConfigSyncManager handles periodic synchronization of config settings
// from the control panel to the fuse box via C1 (ConfigGetSettings) requests
class ConfigSyncManager
{
private:
	static constexpr unsigned long SYNC_TIMEOUT_MS = 5000;
	SerialCommandManager* _computerSerial;
	SerialCommandManager* _linkSerial;
	ConfigController* _configController;
	WarningManager* _warningManager;
	unsigned long _syncInterval;
	unsigned long _retryInterval;
	unsigned long _lastSyncRequest;
	unsigned long _lastConfigReceived;
	bool _enabled;
	bool _syncing;
	Config _configSnapshot;

	void sendSyncRequest();
	void completeSyncIfTimeout(unsigned long now);
	bool hasConfigChanged();
	void saveConfigIfChanged();
	unsigned long getCurrentSyncInterval() const; // Returns appropriate interval based on sync state

public:
	ConfigSyncManager(SerialCommandManager* commandMgrComputer, 
		SerialCommandManager* linkSerial, ConfigController* configController,
		WarningManager* warningManager,
		unsigned long syncInterval = DefaultConfigSyncIntervalMs,
		unsigned long retryInterval = FastConfigSyncRetryMs);

    void update(unsigned long now);

    void requestSync();

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    void setSyncInterval(unsigned long interval);
    unsigned long getSyncInterval() const { return _syncInterval; }

    bool isSyncing() const { return _syncing; }

    void notifyConfigReceived();

};
