#include "BluetoothRelayService.h"

BluetoothRelayService* BluetoothRelayService::_serviceInstance = nullptr;

BluetoothRelayService::BluetoothRelayService(RelayCommandHandler* relayHandler)
    : _relayHandler(relayHandler),
      _relayCount(0),
      _service(nullptr),
      _statesChar(nullptr),
      _setChar(nullptr)
{
    if (_relayHandler)
    {
        _relayCount = _relayHandler->getRelayCount();
	}
}

BluetoothRelayService::~BluetoothRelayService()
{
    // Free BLE objects in reverse creation order
    if (_setChar)
    {
        delete _setChar;
        _setChar = nullptr;
    }

    if (_statesChar)
    {
        delete _statesChar;
        _statesChar = nullptr;
    }

    if (_service)
    {
        delete _service;
        _service = nullptr;
    }

    if (BluetoothRelayService::_serviceInstance == this)
    {
        BluetoothRelayService::_serviceInstance = nullptr;
    }
}

bool BluetoothRelayService::begin()
{
    if (!_relayHandler || _relayCount == 0)
    {
        return false;
    }

    // Create service
    _service = new BLEService(RelayServiceUuid);
    if (!_service)
    {
        return false;
    }

    // Create characteristics
    // States characteristic: read/notify with fixed length = _relayCount (0..7 -> 8 bytes typical)
    _statesChar = new BLECharacteristic(RelayStatesCharUuid, BLERead | BLENotify, _relayCount);
    if (!_statesChar)
    {
        return false;
    }

    // Set characteristic: write without response, payload = [idx, state]
    _setChar = new BLECharacteristic(RelaySetCharUuid, BLEWriteWithoutResponse, 2);
    if (!_setChar)
    {
        return false;
    }

    // Add characteristics to service
    _service->addCharacteristic(*_statesChar);
    _service->addCharacteristic(*_setChar);

    // Initialize states
    {
        uint8_t buf[8] = {};
        loadStates(buf);
        _statesChar->writeValue(buf, _relayCount);
    }

    // Register write callback
    BluetoothRelayService::_serviceInstance = this;
    _setChar->setEventHandler(BLEWritten, BluetoothRelayService::onRelaySetWritten);

    // Register service with BLE stack
    BLE.addService(*_service);

    return true;
}

const char* BluetoothRelayService::getServiceUUID() const
{
    return RelayServiceUuid;
}

const char* BluetoothRelayService::getServiceName() const
{
    return "Relays";
}

void* BluetoothRelayService::getBLEService()
{
    return _service;
}

void BluetoothRelayService::loop(unsigned long /*currentMillis*/)
{
    // No periodic work required; notifications are sent on change via notifyAll().
}

uint8_t BluetoothRelayService::getCharacteristicCount() const
{
    return 2;
}

void BluetoothRelayService::loadStates(uint8_t* buffer)
{
    for (uint8_t i = 0; i < _relayCount; i++)
    {
        buffer[i] = _relayHandler->getRelayStatus(i);
    }
}

void BluetoothRelayService::notifyAll()
{
    if (!_statesChar)
    {
        return;
    }

    uint8_t buf[8] = {};
    loadStates(buf);
    _statesChar->writeValue(buf, _relayCount); // BLENotify triggers notify
}

void BluetoothRelayService::onWriteSet()
{
    if (!_setChar || !_relayHandler)
    {
        return;
    }

    if (_setChar->valueLength() != 2)
    {
        return;
    }

    uint8_t payload[2] = { 0, 0 };
    _setChar->readValue(payload, 2);

    const uint8_t idx = payload[0];
    const uint8_t state = payload[1];

    if (idx >= _relayCount || (state != 0 && state != 1))
    {
        return;
    }

    // Enforce reserved horn relay and bounds inside handler
    RelayResult result = _relayHandler->setRelayStatus(idx, state > 0);
    if (result == RelayResult::Success)
    {
        notifyAll();
    }
}

void BluetoothRelayService::onRelaySetWritten(BLEDevice /*central*/, BLECharacteristic /*characteristic*/)
{
    if (_serviceInstance)
    {
        _serviceInstance->onWriteSet();
    }
}