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
#include "ConfigSyncManager.h"
#include "SystemDefinitions.h"
#include "ConfigManager.h"
#include <string.h>

ConfigSyncManager::ConfigSyncManager(SerialCommandManager* commandMgrComputer,
    SerialCommandManager* linkSerial, 
    ConfigController* configController,
    WarningManager* warningManager,
    unsigned long syncInterval,
    unsigned long retryInterval)
    : _computerSerial(commandMgrComputer),
      _linkSerial(linkSerial),
      _configController(configController),
      _warningManager(warningManager),
      _syncInterval(syncInterval),
      _retryInterval(retryInterval),
      _lastSyncRequest(0),
      _lastConfigReceived(0),
      _enabled(true),
      _syncing(false)
{
    memset(&_configSnapshot, 0, sizeof(_configSnapshot));
}

void ConfigSyncManager::update(unsigned long now)
{
    if (!_enabled || !_linkSerial)
        return;

    // Check if sync is complete (timeout)
    if (_syncing)
    {
        completeSyncIfTimeout(now);
    }

    // Check if sync interval has elapsed
    // Use fast retry interval if sync is failing, normal interval if synced
    unsigned long currentInterval = getCurrentSyncInterval();
    if (!_syncing && currentInterval > 0 && (now - _lastSyncRequest >= currentInterval))
    {
        sendSyncRequest();
    }
}

void ConfigSyncManager::requestSync()
{
    if (_linkSerial && !_syncing)
    {
        sendSyncRequest();
    }
}

void ConfigSyncManager::setEnabled(bool enabled)
{
    _enabled = enabled;
    if (!enabled)
    {
        _syncing = false;
    }
}

void ConfigSyncManager::setSyncInterval(unsigned long interval)
{
    _syncInterval = interval;
}

void ConfigSyncManager::sendSyncRequest()
{
    if (!_linkSerial || !_configController)
        return;
    
    // Snapshot current config before requesting sync
    Config* currentConfig = _configController->getConfigPtr();
    if (currentConfig)
    {
        memcpy(&_configSnapshot, currentConfig, sizeof(Config));
    }
    
    // Send C1 (ConfigGetSettings) to request all config from control panel
    _linkSerial->sendCommand(ConfigGetSettings, "");
    _lastSyncRequest = millis();
    _lastConfigReceived = _lastSyncRequest;
    _syncing = true;
}

void ConfigSyncManager::notifyConfigReceived()
{
    if (_syncing)
    {
        _lastConfigReceived = millis();
    }
}

void ConfigSyncManager::completeSyncIfTimeout(unsigned long now)
{
    // If no config received for SYNC_TIMEOUT_MS, consider sync complete
    if ((now - _lastConfigReceived) >= SYNC_TIMEOUT_MS)
    {
        _syncing = false;

        // Check if ANY config was received during sync
        // If _lastConfigReceived == _lastSyncRequest, nothing was received (Control Panel didn't respond)
        if (_lastConfigReceived == _lastSyncRequest)
        {
            // No config received - Control Panel didn't respond to C1
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::SyncFailed);
            }

            if (_computerSerial)
            {
                _computerSerial->sendError("Config sync failed - no response", "ConfigSync");
            }
        }
        else
        {
            // Config was received - attempt to save if changed
            saveConfigIfChanged();
        }
    }
}

bool ConfigSyncManager::hasConfigChanged()
{
    Config* currentConfig = _configController->getConfigPtr();
    if (!currentConfig)
        return false;
    
    // Compare current config with snapshot
    return memcmp(&_configSnapshot, currentConfig, sizeof(Config)) != 0;
}

void ConfigSyncManager::saveConfigIfChanged()
{
    if (hasConfigChanged())
    {
        // Config has changed - save to EEPROM
        ConfigResult result = _configController->save();

        if (result == ConfigResult::Success)
        {
            // Clear sync warning on successful save
            if (_warningManager)
            {
                _warningManager->clearWarning(WarningType::SyncFailed);
            }

            if (_computerSerial)
            {
                _computerSerial->sendDebug("Config synced and saved", "ConfigSync");
            }
        }
        else
        {
            // Save failed - raise warning
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::SyncFailed);
            }

            if (_computerSerial)
            {
                _computerSerial->sendError("Config sync failed to save", "ConfigSync");
            }
        }
    }
    else
    {
        // No changes detected - clear warning (sync successful but no changes)
        if (_warningManager)
        {
            _warningManager->clearWarning(WarningType::SyncFailed);
        }
    }
}

unsigned long ConfigSyncManager::getCurrentSyncInterval() const
{
    // Use fast retry interval if SyncFailed warning is active
    // Otherwise use normal sync interval
    if (_warningManager && _warningManager->isWarningActive(WarningType::SyncFailed))
    {
        return _retryInterval; // Fast retry (10 seconds)
    }

    return _syncInterval; // Normal interval (5 minutes)
}
