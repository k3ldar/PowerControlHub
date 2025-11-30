#pragma once

#include <Arduino.h>
#include "WifiServer.h"
#include "Config.h"
#include "WarningManager.h"

class WifiController
{
public:
    WifiController(SerialCommandManager* commandMgrComputer, WarningManager* warningManager)
        : _commandMgrComputer(commandMgrComputer),
          _warningManager(warningManager),
          _wifiServer(nullptr),
          _enabled(false)
    {
    }

    ~WifiController()
    {
        disable();
    }

    bool setEnabled(bool enable)
    {
        if (enable == _enabled)
            return true;

        return enable ? enableInternal() : (disable(), true);
    }

    bool isEnabled() const
    {
        return _enabled;
    }

    void applyConfig(const Config* cfg)
    {
        if (!cfg)
        {
            disable();
            return;
        }

        if (!cfg->wifiEnabled)
        {
            disable();
            return;
        }

        // Validate configuration before attempting to enable
        if (!isConfigValid(cfg))
        {
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::WifiInvalidConfig);
            }
            disable();
            return;
        }

        if (enableInternal())
        {
            // Configure based on config settings
            if (cfg->accessMode == 0) // Access Point
            {
                _wifiServer->setAccessPointMode(cfg->_apSSID, cfg->_apPassword);
            }
            else // Client
            {
                _wifiServer->setClientMode(cfg->_apSSID, cfg->_apPassword);
            }

            if (!_wifiServer->begin())
            {
                if (_warningManager)
                {
                    _warningManager->raiseWarning(WarningType::WifiInitFailed);
                }
                disable();
            }
        }
    }

    void update()
    {
        if (_enabled && _wifiServer)
        {
            _wifiServer->update();
        }
    }

    WifiServer* getServer()
    {
        return _wifiServer;
    }

private:
    SerialCommandManager* _commandMgrComputer;
    WarningManager* _warningManager;
    WifiServer* _wifiServer;
    bool _enabled;

    bool isConfigValid(const Config* cfg) const
    {
        if (!cfg)
        {
            return false;
        }

        // Check if SSID is provided and not empty
        if (cfg->_apSSID[0] == '\0')
        {
            return false;
        }

        // In Client mode, password is typically required (though some networks allow empty)
        // In AP mode, password can be optional for open networks
        // You can add additional validation here if needed

        // Validate port number (optional)
        if (cfg->port == 0)
        {
            return false;
        }

        return true;
    }

    bool enableInternal()
    {
        if (_wifiServer != nullptr)
        {
            return true; // Already enabled
        }

        _wifiServer = new WifiServer(_commandMgrComputer);
        
        if (_wifiServer == nullptr)
        {
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::WifiInitFailed);
            }
            return false;
        }

        _enabled = true;
        return true;
    }

    void disable()
    {
        if (_wifiServer != nullptr)
        {
            _wifiServer->end();
            delete _wifiServer;
            _wifiServer = nullptr;
        }

        _enabled = false;
    }
};