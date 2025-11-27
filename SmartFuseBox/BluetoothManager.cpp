#include "BluetoothManager.h"
#include <ArduinoBLE.h>

BluetoothManager::BluetoothManager(BluetoothServiceBase** services, uint8_t serviceCount)
    : _services(services),
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
        Serial.println(F("[BluetoothManager] Error: Device name is required"));
        return false;
    }

    _deviceName = deviceName;

    // Initialize BLE
    if (!BLE.begin())
    {
        Serial.println(F("[BluetoothManager] Error: Failed to initialize BLE"));
        return false;
    }

    // Set device name
    BLE.setLocalName(deviceName);
    BLE.setDeviceName(deviceName);

    // Initialize all services
    if (!initializeServices())
    {
        Serial.println(F("[BluetoothManager] Error: Service initialization failed"));
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

    Serial.print(F("[BluetoothManager] Initialized with "));
    Serial.print(_serviceCount);
    Serial.println(F(" services"));

    return true;
}

bool BluetoothManager::initializeServices()
{
    for (uint8_t i = 0; i < _serviceCount; i++)
    {
        BluetoothServiceBase* service = _services[i];

        if (!service)
        {
            Serial.print(F("[BluetoothManager] Error: Service at index "));
            Serial.print(i);
            Serial.println(F(" is null"));
            return false;
        }

        Serial.print(F("[BluetoothManager] Initializing service: "));
        Serial.println(service->getServiceName());

        if (!service->begin())
        {
            Serial.print(F("[BluetoothManager] Error: Failed to initialize service: "));
            Serial.println(service->getServiceName());
            return false;
        }

        Serial.print(F("[BluetoothManager] Service registered: "));
        Serial.print(service->getServiceName());
        Serial.print(F(" (UUID: "));
        Serial.print(service->getServiceUUID());
        Serial.print(F(", "));
        Serial.print(service->getCharacteristicCount());
        Serial.println(F(" characteristics)"));
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
            Serial.println(F("[BluetoothManager] Client connected"));
        }
        else
        {
            Serial.println(F("[BluetoothManager] Client disconnected"));

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

    Serial.print(F("[BluetoothManager] Advertising as '"));
    Serial.print(_deviceName);
    Serial.println(F("'"));
}

void BluetoothManager::stopAdvertising()
{
    if (!_isAdvertising)
    {
        return;
    }

    BLE.stopAdvertise();
    _isAdvertising = false;

    Serial.println(F("[BluetoothManager] Advertising stopped"));
}