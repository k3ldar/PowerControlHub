#pragma once

#include <Arduino.h>
#include <ArduinoBLE.h>
#include "BluetoothManager.h"
#include "BluetoothSystemService.h"
#include "BluetoothSensorService.h"
#include "BluetoothRelayService.h"
#include "WarningType.h"
#include "RelayController.h"

class WarningManager;
class SystemCommandHandler;
class SensorCommandHandler;

class BluetoothController
{
public:
    BluetoothController(SystemCommandHandler* systemHandler,
                        SensorCommandHandler* sensorHandler,
		                RelayController* relayController,
                        WarningManager* warningManager,
                        SerialCommandManager* commandMgrComputer)
        : _systemHandler(systemHandler),
          _sensorHandler(sensorHandler),
		  _relayController(relayController),
          _warningManager(warningManager),
		  _commandMgrComputer(commandMgrComputer),
          _manager(nullptr),
          _services(nullptr),
          _serviceCount(0),
          _systemService(nullptr),
          _sensorService(nullptr),
          _enabled(false)
    {
    }

    ~BluetoothController()
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
        setEnabled(cfg && cfg->bluetoothEnabled);
    }

    void loop()
    {
        if (_enabled && _manager)
        {
            _manager->loop();
        }
    }

private:
    SystemCommandHandler* _systemHandler;
    SensorCommandHandler* _sensorHandler;
    RelayController* _relayController;
    WarningManager* _warningManager;
	SerialCommandManager* _commandMgrComputer;

    BluetoothManager* _manager;
    BluetoothServiceBase** _services;
    uint8_t _serviceCount;

    BluetoothSystemService* _systemService;
    BluetoothSensorService* _sensorService;
	BluetoothRelayService* _relayService;

    bool _enabled;

    bool enableInternal()
    {
        // Initialize BLE stack
        if (!BLE.begin())
        {
            if (_warningManager) _warningManager->raiseWarning(WarningType::BluetoothInitFailed);
            return false;
        }

        // Create services
        _systemService = new BluetoothSystemService(_systemHandler);
        _sensorService = new BluetoothSensorService(_sensorHandler);
		_relayService = new BluetoothRelayService(_relayController);

        _serviceCount = 3U;
        _services = new BluetoothServiceBase*[_serviceCount];
        _services[0] = _systemService;
        _services[1] = _sensorService;
        _services[2] = _relayService;

        // Create manager
        _manager = new BluetoothManager(_commandMgrComputer, _warningManager, _services, _serviceCount);

        if (!_manager->begin("Smart Fuse Box"))
        {
            if (_warningManager) _warningManager->raiseWarning(WarningType::BluetoothInitFailed);
            disable(); // clean up partial init
            return false;
        }

        _systemService->notifyInitialized();
        _enabled = true;
        return true;
    }

    void disable()
    {
        // Stop BLE stack first
        BLE.end();

        // Delete manager first (may reference services array)
        if (_manager)
        {
            delete _manager;
            _manager = nullptr;
        }

        // Delete service pointer array
        if (_services)
        {
            delete[] _services;
            _services = nullptr;
            _serviceCount = 0;
        }

        // Delete services
        if (_sensorService)
        {
            delete _sensorService;
            _sensorService = nullptr;
        }

        if (_systemService)
        {
            delete _systemService;
            _systemService = nullptr;
        }

        if (_relayService)
        {
            delete _relayService;
            _relayService = nullptr;
        }

        _enabled = false;
    }
};