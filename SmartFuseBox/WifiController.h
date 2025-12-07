#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>

#include "INetworkCommandHandler.h"
#include "WifiServer.h"
#include "Config.h"
#include "WarningManager.h"

class WifiController
{
public:
    WifiController(SerialCommandManager* commandMgrComputer, WarningManager* warningManager)
        : _commandMgrComputer(commandMgrComputer),
          _wifiServer(nullptr),
          _enabled(false),
		_handlerObjects(nullptr),
		_handlerCount(0),
		_port(80),
        _jsonVisitors(nullptr),
        _jsonVisitorCount(0)
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
                _wifiServer->setAccessPointMode(cfg->apSSID, cfg->apPassword, cfg->apIpAddress);
            }
            else // Client
            {
                _wifiServer->setClientMode(cfg->apSSID, cfg->apPassword);
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

    void registerHandlers(INetworkCommandHandler** handlers, size_t handlerCount)
    {
        if (_handlerObjects)
        {
            delete[] _handlerObjects;
            _handlerObjects = nullptr;
            _handlerCount = 0;
        }

        _handlerCount = handlerCount;
        _handlerObjects = new INetworkCommandHandler * [_handlerCount];

        for (size_t i = 0; i < _handlerCount; i++)
        {
            _handlerObjects[i] = handlers[i];
        }
    }

    WifiServer* getServer()
    {
        return _wifiServer;
    }

    void registerJsonVisitors(JsonVisitor** jsonVisitors, uint8_t jsonVisitorCount)
    {
        // owned by caller so no need to clean up
        _jsonVisitorCount = jsonVisitorCount;
        _jsonVisitors = jsonVisitors;
    }

private:
    SerialCommandManager* _commandMgrComputer;
    WarningManager* _warningManager;
    WifiServer* _wifiServer;
    bool _enabled;
    INetworkCommandHandler** _handlerObjects;
    uint8_t _handlerCount = 0;
	uint16_t _port = 80;
    JsonVisitor** _jsonVisitors;
    uint8_t _jsonVisitorCount;

    bool isConfigValid(const Config* cfg) const
    {
        if (!cfg)
        {
            return false;
        }

        // Check if SSID is provided and not empty
        if (cfg->apSSID[0] == '\0')
        {
            return false;
        }

        // In Client mode, password is typically required (though some networks allow empty)
        // In AP mode, password can be optional for open networks
        // You can add additional validation here if needed

        // Validate port number (optional)
        if (cfg->wifiPort == 0)
        {
            return false;
        }

        if (cfg->accessMode == 0) // Access Point
        {
            // If apIpAddress is empty, allow using default IP (considered valid)
            if (cfg->apIpAddress[0] != '\0') {
                IPAddress testIp;
                if (!testIp.fromString(cfg->apIpAddress))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool enableInternal()
    {
        if (_wifiServer != nullptr)
        {
            return true; // Already enabled
        }

        _wifiServer = new WifiServer(_commandMgrComputer, _warningManager, _port, _handlerObjects, _handlerCount, _jsonVisitors, _jsonVisitorCount);
        
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