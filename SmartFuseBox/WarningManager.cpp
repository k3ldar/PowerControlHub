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
#include "WarningManager.h"

#if defined(BOAT_CONTROL_PANEL)
#include "ToneManager.h"
#include "DateTimeManager.h"
#endif

// Define sensor warning mask (bits 20-31)
constexpr uint32_t SENSOR_WARNING_MASK = 0xFFF00000U; // Bits 20-31 set

WarningManager::WarningManager(SerialCommandManager* commandMgr, unsigned long heartbeatInterval, 
	unsigned long heartbeatTimeout
#if defined(BOAT_CONTROL_PANEL)
	, RgbLedFade* warningStatus,
	ToneManager* toneManager
#endif
	)

	: _commandMgr(commandMgr),
#if defined(BOAT_CONTROL_PANEL)
	_warningStatus(warningStatus),
	_toneManager(toneManager),
#endif
	  _localWarnings(0),
	  _remoteWarnings(0),
#if defined(BOAT_CONTROL_PANEL)
	  _previousWarnings(0),
	  _lastTonePlayed(0),
#endif
	  _heartbeatInterval(heartbeatInterval),
	  _heartbeatTimeout(heartbeatTimeout),
	  _lastHeartbeatSent(0),
	  _lastHeartbeatReceived(0),
	  _heartbeatEnabled(heartbeatInterval > 0)
{
#if defined(BOAT_CONTROL_PANEL)
	updateLedStatus();
#endif
}

void WarningManager::update(unsigned long now)
{
	if (_heartbeatEnabled && _commandMgr)
	{
		updateConnection(now);
	}

#if defined(BOAT_CONTROL_PANEL)
	// Update LED status every loop iteration
	updateLedStatus();

	// Handle warning tone alerts
	if (_toneManager)
	{
		uint32_t currentWarnings = _localWarnings | _remoteWarnings;

		// Check if warnings are active
		if (currentWarnings != 0)
		{
			// Detect NEW warnings (bits that weren't set before)
			uint32_t newWarnings = currentWarnings & ~_previousWarnings;

			if (newWarnings != 0)
			{
				// New warning(s) appeared - play bad tone immediately
				_toneManager->play(ToneType::Bad);
				_lastTonePlayed = now;
			}
			else if (_lastTonePlayed > 0 && !_toneManager->isPlaying())
			{
				// Check if we need to repeat the tone
				unsigned long repeatInterval = _toneManager->getRepeatIntervalMs();
				if (repeatInterval == 0)
					repeatInterval = 5000; // Fallback if 0 configured

				if (now - _lastTonePlayed >= repeatInterval)
				{
					_toneManager->play(ToneType::Bad);
					_lastTonePlayed = now;
				}
			}
		}
		else
		{
			// No warnings active - stop any playing tone and reset timer
			if (_toneManager->isPlaying())
			{
				_toneManager->stop();
			}

			_lastTonePlayed = 0;
		}

		_previousWarnings = currentWarnings;
	}
#endif
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
	bool wasActive = (_localWarnings & warningBit) != 0;
	_localWarnings |= warningBit;

	// Broadcast warning change if it's new
	if (!wasActive)
	{
		broadcastWarningChange(type, true);
	}

	// Auto-raise SensorFailure for any sensor-related warning (bit 20+)
	if (warningBit & SENSOR_WARNING_MASK)
	{
		uint32_t sensorFailureBit = static_cast<uint32_t>(WarningType::SensorFailure);
		bool sensorFailureWasActive = (_localWarnings & sensorFailureBit) != 0;
		_localWarnings |= sensorFailureBit;

		// Broadcast SensorFailure if it wasn't already active
		if (!sensorFailureWasActive)
		{
			broadcastWarningChange(WarningType::SensorFailure, true);
		}
	}

#if defined(BOAT_CONTROL_PANEL)
	updateLedStatus();
#endif
}

void WarningManager::clearWarning(WarningType type)
{
	if (type == WarningType::None)
		return;

	uint32_t warningBit = static_cast<uint32_t>(type);
	bool wasActive = (_localWarnings & warningBit) != 0;
	_localWarnings &= ~warningBit;

	// Broadcast warning change if it was active
	if (wasActive)
	{
		broadcastWarningChange(type, false);
	}

	// Auto-clear SensorFailure only if no sensor warnings remain (check bits 20+)
	if (warningBit & SENSOR_WARNING_MASK)
	{
		// Check if any other sensor warnings are still active (excluding SensorFailure itself)
		uint32_t otherSensorWarnings = _localWarnings & SENSOR_WARNING_MASK & ~static_cast<uint32_t>(WarningType::SensorFailure);
		
		if (otherSensorWarnings == 0)
		{
			uint32_t sensorFailureBit = static_cast<uint32_t>(WarningType::SensorFailure);
			bool sensorFailureWasActive = (_localWarnings & sensorFailureBit) != 0;
			_localWarnings &= ~sensorFailureBit;

			// Broadcast SensorFailure clear if it was active
			if (sensorFailureWasActive)
			{
				broadcastWarningChange(WarningType::SensorFailure, false);
			}
		}
	}

#if defined(BOAT_CONTROL_PANEL)
	updateLedStatus();
#endif
}

void WarningManager::clearAllWarnings()
{
	_localWarnings = 0;
	_remoteWarnings = 0;
	
#if defined(BOAT_CONTROL_PANEL)
	updateLedStatus();
#endif
}

bool WarningManager::hasWarnings() const
{
	return (_localWarnings | _remoteWarnings) != 0;
}

bool WarningManager::isWarningActive(WarningType type) const
{
	if (type == WarningType::None)
		return false;
	
	uint32_t warningBit = static_cast<uint32_t>(type);
	// Warning is active if it's in EITHER local or remote
	return ((_localWarnings | _remoteWarnings) & warningBit) != 0;
}

void WarningManager::sendHeartbeat()
{
	if (_commandMgr)
	{
		// Include local warnings in heartbeat
		char params[64];

#if defined(BOAT_CONTROL_PANEL)
		snprintf_P(params, sizeof(params), PSTR("w=%s%lx;t=%lu"),
			HexPrefix, _localWarnings, DateTimeManager::getCurrentTime());
#else
		snprintf_P(params, sizeof(params), PSTR("w=%s%lx"),
			HexPrefix, _localWarnings);
#endif
		_commandMgr->sendCommand(SystemHeartbeatCommand, params);
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
		// Protect against race condition: if lastHeartbeatReceived is in the future
		// (due to notifyHeartbeatAck calling millis() after we captured 'now'),
		// treat it as just received (delta = 0)
		unsigned long delta = (_lastHeartbeatReceived > now) ? 0 : (now - _lastHeartbeatReceived);

		bool connected = (_lastHeartbeatReceived > 0) && (delta < _heartbeatTimeout);

		bool wasConnected = !isWarningActive(WarningType::ConnectionLost);

		// Only update warning state if connection status changed
		if (connected && !wasConnected)
		{
			clearWarning(WarningType::ConnectionLost);

#if defined(BOAT_CONTROL_PANEL)
			// Send time sync when connection is established
			if (_commandMgr && DateTimeManager::isTimeSet())
			{
				char buffer[20];
				snprintf_P(buffer, sizeof(buffer), PSTR("v=%lu"), DateTimeManager::getCurrentTime());
				_commandMgr->sendCommand(SystemSetDateTime, buffer);
			}
#endif
		}
		else if (!connected && wasConnected)
		{
			raiseWarning(WarningType::ConnectionLost);
			_remoteWarnings = 0; // Clear remote warnings on connection loss
		}
	}
}

uint32_t WarningManager::getActiveWarningsMask() const
{
	// Return combined view of local and remote warnings
	return _localWarnings | _remoteWarnings;
}

uint32_t WarningManager::getLocalWarningsMask() const
{
	return _localWarnings;
}

uint32_t WarningManager::getRemoteWarningsMask() const
{
	return _remoteWarnings;
}

void WarningManager::updateRemoteWarnings(uint32_t remoteWarningMask)
{
	_remoteWarnings = remoteWarningMask;

#if defined(BOAT_CONTROL_PANEL)
	updateLedStatus();
#endif
}

void WarningManager::broadcastWarningChange(WarningType type, bool isActive)
{
	if (!_commandMgr || type == WarningType::None)
		return;

	// Format warning type as hex string (e.g., "0x800")
	char warningHex[12];
	uint32_t warningValue = static_cast<uint32_t>(type);
	snprintf_P(warningHex, sizeof(warningHex), PSTR("%s%lx"), HexPrefix, warningValue);

	// Format as W2:<hex>=<0|1>
	char params[32];
	snprintf_P(params, sizeof(params), PSTR("%s=%d"), warningHex, isActive ? 1 : 0);

	// Send W2 command via LINK
	_commandMgr->sendCommand(WarningStatus, params);
}

#if defined(BOAT_CONTROL_PANEL)

void WarningManager::updateLedStatus()
{
    if (!_warningStatus)
        return;
    
    uint32_t allWarnings = _localWarnings | _remoteWarnings;
    
    // Set warning state based on active warnings
    _warningStatus->setWarning(allWarnings != 0);
}
#endif