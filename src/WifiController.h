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

#include "Local.h"
#include "IWifiController.h"
#include "INetworkCommandHandler.h"
#include "WifiServer.h"
#include "WifiRadioBridge.h"
#include "Config.h"
#include "WarningManager.h"
#include "MessageBus.h"

constexpr uint64_t LedUpdateIntevalMs = 300;
class WifiController : public IWifiController
{
private:
	MessageBus* _messageBus;
    SerialCommandManager* _commandMgrComputer;
    WarningManager* _warningManager;
    WifiServer* _wifiServer;
    bool _enabled;
    INetworkCommandHandler** _handlerObjects;
    uint8_t _handlerCount = 0;
    uint16_t _port = 80;
    NetworkJsonVisitor** _jsonVisitors;
    uint8_t _jsonVisitorCount;
    uint64_t _lastUpdateTime;

    bool isConfigValid(const Config* cfg) const
    {
        if (!cfg)
        {
            return false;
        }

        // Check if SSID is provided and not empty
        if (cfg->network.ssid[0] == '\0')
        {
            return false;
        }

        // In Client mode, password is typically required (though some networks allow empty)
        // In AP mode, password can be optional for open networks
        // You can add additional validation here if needed

        // Validate port number (optional)
        if (cfg->network.port == 0)
        {
            return false;
        }

        if (cfg->network.accessMode == WifiMode::AccessPoint)
        {
            // If apIpAddress is empty, allow using default IP (considered valid)
            if (cfg->network.apIpAddress[0] != '\0')
            {
                IPAddress testIp;
                if (!testIp.fromString(cfg->network.apIpAddress))
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

        _wifiServer = new WifiServer(_messageBus, _commandMgrComputer, _warningManager,
            _port, _handlerObjects, _handlerCount, _jsonVisitors, _jsonVisitorCount, &_radio);

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
public:
    WifiController(MessageBus* messageBus, SerialCommandManager* commandMgrComputer, WarningManager* warningManager)
		: _messageBus(messageBus),
        _commandMgrComputer(commandMgrComputer),
		_warningManager(warningManager),
        _wifiServer(nullptr),
        _enabled(false),
		_handlerObjects(nullptr),
		_handlerCount(0),
		_port(80),
        _jsonVisitors(nullptr),
        _jsonVisitorCount(0),
		_lastUpdateTime(0)
    {
    }

    ~WifiController()
    {
        disable();
    }

    bool setEnabled(bool enable) override
    {
        if (enable == _enabled)
            return true;

        return enable ? enableInternal() : (disable(), true);
    }

    bool isEnabled() const override
    {
        return _enabled;
    }

    void applyConfig(const Config* cfg) override
    {
        if (!cfg)
        {
            disable();
            return;
        }

        if (!cfg->network.wifiEnabled)
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
            if (cfg->network.accessMode == WifiMode::AccessPoint)
            {
                _wifiServer->setAccessPointMode(cfg->network.ssid, cfg->network.password, cfg->network.apIpAddress);
            }
            else // Client
            {
                _wifiServer->setClientMode(cfg->network.ssid, cfg->network.password);
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

	void update(uint64_t now) override
	{
		if (_enabled && _wifiServer)
		{
			_wifiServer->update();

			if (now - _lastUpdateTime >= LedUpdateIntevalMs)
			{
				_lastUpdateTime = now;
				_messageBus->publish<WifiConnectionStateChanged>(_wifiServer->getConnectionState());
				_messageBus->publish<WifiSignalStrengthChanged>(_wifiServer->getSignalStrength());
			}
		}
	}

	void registerHandlers(INetworkCommandHandler** handlers, size_t handlerCount) override
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

	WifiServer* getServer() override
	{
		return _wifiServer;
	}

	IWifiRadio* getRadio()
	{
		return &_radio;
	}

	void registerJsonVisitors(NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount) override
	{
		// owned by caller so no need to clean up
		_jsonVisitorCount = jsonVisitorCount;
		_jsonVisitors = jsonVisitors;
	}

	WifiConnectionState getConnectionState() const override
	{
		if (_wifiServer)
		{
			return _wifiServer->getConnectionState();
		}
		return WifiConnectionState::Disconnected;
	}

	void getIpAddress(char* buffer, size_t bufferSize) const override
	{
		if (_wifiServer && buffer && bufferSize > 0)
		{
			_wifiServer->getIpAddress(buffer, bufferSize);
		}
		else if (buffer && bufferSize > 0)
		{
			buffer[0] = '\0';
		}
	}

	PlatformWifiRadio _radio;
};