#include <ArduinoBLE.h>
#include "BluetoothManager.h"

BluetoothManager::BluetoothManager(SerialCommandManager* commandMgrComputer, WarningManager* warningManager, BluetoothServiceBase** services, uint8_t serviceCount)
	: SingleLoggerSupport(commandMgrComputer),
	_warningManager(warningManager),
    _services(services),
    _serviceCount(serviceCount),
    _server(nullptr),
    _isAdvertising(false),
    _deviceConnected(false),
    _lastLoopTime(0),
    _deviceName(nullptr)
{
}

BluetoothManager::~BluetoothManager()
{
    // ArduinoBLE cleanup is handled by BLE.end() if needed
    // Services are not owned by manager (user manages lifetime)
}

bool BluetoothManager::begin(const char* deviceName)
{
    if (!deviceName || deviceName[0] == '\0')
    {
        sendError(F("Bluetooth device name is required"), F("BluetoothManager"));

		if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::BluetoothInitFailed);
        }

        return false;
    }

    _deviceName = deviceName;

    // Initialize BLE
    if (!BLE.begin())
    {
        sendError(F("Failed to initialize BLE"), F("BluetoothManager"));

        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::BluetoothInitFailed);
        }

        return false;
    }

    // Set device name
    BLE.setLocalName(deviceName);
    BLE.setDeviceName(deviceName);

    // Initialize all services
    if (!initializeServices())
    {
        sendError(F("Service initialization failed"), F("BluetoothManager"));

        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::BluetoothInitFailed);
        }

        return false;
    }

    // Set event handlers
    BLE.setEventHandler(BLEConnected, [](BLEDevice central) {
        (void)central;
        // Connection handled in loop()
    });

    BLE.setEventHandler(BLEDisconnected, [](BLEDevice central) {
        (void)central;
        // Disconnection handled in loop()
    });

    // Start advertising
    startAdvertising();

    char debugMsg[64];
    snprintf(debugMsg, sizeof(debugMsg), "Initialized with %d services", _serviceCount);
    sendDebug(debugMsg, F("BluetoothManager"));

    return true;
}

bool BluetoothManager::initializeServices()
{
    for (uint8_t i = 0; i < _serviceCount; i++)
    {
        BluetoothServiceBase* service = _services[i];

        if (!service)
        {
            return false;
        }

        if (!service->begin())
        {
            return false;
        }
    }

    return true;
}

void BluetoothManager::loop()
{
    unsigned long currentMillis = millis();

    // Poll for BLE events
    BLE.poll();

    // Update connection status
    bool connected = BLE.connected();

    // Handle connection state changes
    if (connected != _deviceConnected)
    {
        _deviceConnected = connected;

        if (_deviceConnected)
        {
            sendDebug(F("Client connected"), F("BluetoothManager"));
        }
        else
        {
            sendDebug(F("Client disconnected"), F("BluetoothManager"));

            // Restart advertising after disconnect
            startAdvertising();
        }
    }

    // Call loop on all services
    for (uint8_t i = 0; i < _serviceCount; i++)
    {
        if (_services[i])
        {
            _services[i]->loop(currentMillis);
        }
    }

    _lastLoopTime = currentMillis;
}

bool BluetoothManager::isConnected() const
{
    return _deviceConnected;
}

uint8_t BluetoothManager::getServiceCount() const
{
    return _serviceCount;
}

BluetoothServiceBase* BluetoothManager::getService(uint8_t index) const
{
    if (index >= _serviceCount)
    {
        return nullptr;
    }

    return _services[index];
}

void BluetoothManager::startAdvertising()
{
    if (_isAdvertising)
    {
        return;
    }

    // Set advertised services
    for (uint8_t i = 0; i < _serviceCount; i++)
    {
        if (_services[i])
        {
            BLE.setAdvertisedService(*static_cast<BLEService*>(_services[i]->getBLEService()));
        }
    }

    // Start advertising
    BLE.advertise();

    _isAdvertising = true;
	char debugMsg[64];
	snprintf(debugMsg, sizeof(debugMsg), "Advertising started as '%s'", _deviceName);
    sendDebug(debugMsg, F("BluetoothManager"));
}

void BluetoothManager::stopAdvertising()
{
    if (!_isAdvertising)
    {
        return;
    }

    BLE.stopAdvertise();
    _isAdvertising = false;

    sendDebug(F("Advertising stopped"), F("BluetoothManager"));
}

bool BluetoothManager::isAdvertising()
{
	return _isAdvertising;
}