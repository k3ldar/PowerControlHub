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
#include "SystemFunctions.h"

#include "ToneManager.h"
#include "DateTimeManager.h"

// Define sensor warning mask (bits 20-31)
constexpr uint32_t SENSOR_WARNING_MASK = 0xFFF00000U; // Bits 20-31 set

WarningManager::WarningManager(
    )
	: _warningStatus(nullptr),
	  _toneManager(nullptr),
	  _localWarnings(0),
	  _previousWarnings(0),
	  _lastTonePlayed(0)
{
	updateLedStatus();
}

void WarningManager::setWarningStatus(RgbLedFade* warningStatus)
{
	_warningStatus = warningStatus;
	updateLedStatus();
}

void WarningManager::setToneManager(ToneManager* toneManager)
{
	_toneManager = toneManager;
}

void WarningManager::update(uint64_t now)
{
	// Update LED status every loop iteration
	updateLedStatus();

	// Handle warning tone alerts
	if (_toneManager)
	{
		uint32_t currentWarnings = _localWarnings;

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
				uint64_t repeatInterval = _toneManager->getRepeatIntervalMs();
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
}

void WarningManager::raiseWarning(WarningType type)
{
	if (type == WarningType::None)
		return;

	uint32_t warningBit = static_cast<uint32_t>(type);
	_localWarnings |= warningBit;

	// Auto-raise SensorFailure for any sensor-related warning (bit 20+)
	if (warningBit & SENSOR_WARNING_MASK)
	{
		_localWarnings |= static_cast<uint32_t>(WarningType::SensorFailure);
	}

	updateLedStatus();
}

void WarningManager::clearWarning(WarningType type)
{
	if (type == WarningType::None)
		return;

	uint32_t warningBit = static_cast<uint32_t>(type);
	_localWarnings &= ~warningBit;

	// Auto-clear SensorFailure only if no sensor warnings remain (check bits 20+)
	if (warningBit & SENSOR_WARNING_MASK)
	{
		uint32_t otherSensorWarnings = _localWarnings & SENSOR_WARNING_MASK & ~static_cast<uint32_t>(WarningType::SensorFailure);

		if (otherSensorWarnings == 0)
		{
			_localWarnings &= ~static_cast<uint32_t>(WarningType::SensorFailure);
		}
	}

	updateLedStatus();
}

void WarningManager::clearAllWarnings()
{
	_localWarnings = 0;
	
	updateLedStatus();
}

bool WarningManager::hasWarnings() const
{
	return _localWarnings != 0;
}

bool WarningManager::isWarningActive(WarningType type) const
{
	if (type == WarningType::None)
		return false;

	uint32_t warningBit = static_cast<uint32_t>(type);
	return (_localWarnings & warningBit) != 0;
}

uint32_t WarningManager::getActiveWarningsMask() const
{
	return _localWarnings;
}

void WarningManager::updateLedStatus()
{
    if (!_warningStatus)
        return;
    
	_warningStatus->setWarning(_localWarnings != 0);
}
