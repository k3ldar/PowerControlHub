#include "WarningManager.h"

// Define sensor warning mask (bits 20-31)
constexpr uint32_t SENSOR_WARNING_MASK = 0xFFF00000U; // Bits 20-31 set

WarningManager::WarningManager(SerialCommandManager* commandMgr, unsigned long heartbeatInterval, unsigned long heartbeatTimeout)
    : _commandMgr(commandMgr),
      _activeWarnings(0),
      _heartbeatInterval(heartbeatInterval),
      _heartbeatTimeout(heartbeatTimeout),
      _lastHeartbeatSent(0),
      _lastHeartbeatReceived(0),
      _heartbeatEnabled(heartbeatInterval > 0)
{
}

void WarningManager::update(unsigned long now)
{
    if (_heartbeatEnabled && _commandMgr)
    {
        updateConnection(now);
    }
}

void WarningManager::notifyHeartbeatAck()
{
    _lastHeartbeatReceived = millis();
    
    // Clear the connection lost warning (connection is now established)
    clearWarning(WarningType::ConnectionLost);
}

void WarningManager::raiseWarning(WarningType type)
{
    if (type == WarningType::None)
        return;
    
    uint32_t warningBit = static_cast<uint32_t>(type);
    _activeWarnings |= warningBit;
    
    // Auto-raise SensorFailure for any sensor-related warning (bit 20+)
    if (warningBit & SENSOR_WARNING_MASK) {
        _activeWarnings |= static_cast<uint32_t>(WarningType::SensorFailure);
    }
}

void WarningManager::clearWarning(WarningType type)
{
    if (type == WarningType::None)
        return;
    
    uint32_t warningBit = static_cast<uint32_t>(type);
    _activeWarnings &= ~warningBit;
    
    // Auto-clear SensorFailure only if no sensor warnings remain (check bits 20+)
    if (warningBit & SENSOR_WARNING_MASK) {
        // Check if any other sensor warnings are still active (excluding SensorFailure itself)
        uint64_t otherSensorWarnings = _activeWarnings & SENSOR_WARNING_MASK & ~static_cast<uint64_t>(WarningType::SensorFailure);
        if (otherSensorWarnings == 0) {
            _activeWarnings &= ~static_cast<uint32_t>(WarningType::SensorFailure);
        }
    }
}

void WarningManager::clearAllWarnings()
{
    _activeWarnings = 0;
}

bool WarningManager::hasWarnings() const
{
    return _activeWarnings != 0;
}

bool WarningManager::isWarningActive(WarningType type) const
{
    if (type == WarningType::None)
        return false;
    
    uint32_t warningBit = static_cast<uint32_t>(type);
    return (_activeWarnings & warningBit) != 0;
}

void WarningManager::sendHeartbeat()
{
    if (_commandMgr)
    {
        _commandMgr->sendCommand(SystemHeartbeatCommand, "");
    }
}

void WarningManager::updateConnection(unsigned long now)
{
    // Send heartbeat if interval elapsed
    if (now - _lastHeartbeatSent >= _heartbeatInterval)
    {
        sendHeartbeat();
        _lastHeartbeatSent = now;
    }

    // Check for timeout (only after we've sent at least one heartbeat)
    if (_lastHeartbeatSent > 0 || now >= _heartbeatTimeout)
    {
        bool connected = (_lastHeartbeatReceived > 0) && 
                        (now - _lastHeartbeatReceived) < _heartbeatTimeout;

        // Update warning state based on connection status
        if (connected)
        {
            clearWarning(WarningType::ConnectionLost);
        }
        else
        {
            raiseWarning(WarningType::ConnectionLost);
        }
    }
}

uint32_t WarningManager::getActiveWarningsMask() const
{
    return _activeWarnings;
}
