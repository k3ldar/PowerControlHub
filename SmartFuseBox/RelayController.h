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

#include "SystemDefinitions.h"
#include "ConfigManager.h"
#include "MessageBus.h"

enum class RelayResult : uint8_t
{
	Success = 0,
	InvalidIndex = 1,
	Reserved = 2
};


class RelayController
{
private:
	MessageBus* _messageBus;
	bool* _relayStatus;
	uint8_t* _relays;
	uint8_t _relayCount;

	bool isHornRelay(uint8_t relayIndex) const
	{
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr || relayIndex >= _relayCount)
			return false;
		return config->relay.relays[relayIndex].actionType == RelayActionType::Horn;
	}

	bool isRelayDisabled(uint8_t relayIndex) const
	{
		return _relays[relayIndex] == PinDisabled;
	}

	RelayResult setRelayStatus(uint8_t relayIndex, bool isOn)
	{
		if (relayIndex >= _relayCount)
		{
			return RelayResult::InvalidIndex;
		}

		if (isHornRelay(relayIndex))
		{
			return RelayResult::Reserved;
		}

		// Compute which relays actually change state so subscribers can be notified
		uint8_t changedMask = 0;

		// Primary relay — only write hardware if the pin is assigned
		if (_relayStatus[relayIndex] != isOn)
		{
			_relayStatus[relayIndex] = isOn;
			if (!isRelayDisabled(relayIndex))
			{
				digitalWrite(_relays[relayIndex], isOn ? LOW : HIGH);
			}
			changedMask |= (1 << relayIndex);
		}

		Config* config = ConfigManager::getConfigPtr();

		if (config != nullptr)
		{
			// Check for linked relays and update only if their state changes
				for (uint8_t i = 0; i < ConfigMaxLinkedRelays; i++)
				{
					if (config->relay.linkedRelays[i][0] == relayIndex)
					{
						uint8_t linkedRelay = config->relay.linkedRelays[i][1];
						if (linkedRelay < _relayCount && !isRelayDisabled(linkedRelay))
						{
							if (_relayStatus[linkedRelay] != isOn)
							{
								_relayStatus[linkedRelay] = isOn;
								digitalWrite(_relays[linkedRelay], isOn ? LOW : HIGH);
								changedMask |= (1 << linkedRelay);
							}
						}
					}
				}
		}

		if (_messageBus && changedMask)
		{
			// Publish bitmask of relays that actually changed state
			_messageBus->publish<RelayStatusChanged>(changedMask);
		}

		return RelayResult::Success;
	}

public:
	RelayController(MessageBus* messageBus, const uint8_t* relayPins, uint8_t totalRelays)
		: _messageBus(messageBus), _relayStatus(nullptr), _relays(nullptr), _relayCount(totalRelays)
	{
		_relays = new uint8_t[_relayCount];
		memcpy(_relays, relayPins, sizeof(uint8_t)* _relayCount);

		_relayStatus = new bool[_relayCount];

		for (uint8_t i = 0; i < totalRelays; i++)
		{
			_relayStatus[i] = false;
		}
	}

	~RelayController()
	{
		delete[] _relayStatus;
		delete[] _relays;
	}

	void setup()
	{
		for (uint8_t i = 0; i < _relayCount; i++)
		{
			_relayStatus[i] = false;
			if (!isRelayDisabled(i))
			{
				pinMode(_relays[i], OUTPUT);
				digitalWrite(_relays[i], HIGH);
			}
		}
	}

	CommandResult turnAllRelaysOff()
	{
		for (int i = 0; i < _relayCount; i++)
		{
			setRelayStatus(i, false);
		}
		
		return CommandResult::ok();
	}

	CommandResult turnAllRelaysOn()
	{
		for (int i = 0; i < _relayCount; i++)
		{
			setRelayStatus(i, true);
		}
		return CommandResult::ok();
	}

	CommandResult setRelayState(uint8_t relayIndex, bool state)
	{
		RelayResult result = setRelayStatus(relayIndex, state);

		if (result == RelayResult::Success)
		{
			// Return actual state for confirmation
			return CommandResult::okStatus(static_cast<uint8_t>(_relayStatus[relayIndex] ? 1 : 0));
		}

		return CommandResult::error(static_cast<uint8_t>(result));
	}

	CommandResult getRelayStatus(uint8_t relayIndex)
	{
		uint8_t status = PinDisabled;
		if (relayIndex < _relayCount)
		{
			status = _relayStatus[relayIndex] ? 1 : 0;
		}

		return CommandResult::okStatus(status);
	}

	CommandResult getAllRelayStates() const
	{
		uint8_t bitmask = 0;
		for (uint8_t i = 0; i < _relayCount && i < 8; i++)
		{
			if (_relayStatus[i])
			{
				bitmask |= (1 << i);
			}
		}

		return CommandResult::okStatus(bitmask);
	}

	uint8_t getRelayCount() const { return _relayCount; }

	bool isDisabled(uint8_t relayIndex) const
	{
		if (relayIndex >= _relayCount)
			return true;

		return isRelayDisabled(relayIndex);
	}

	// Refresh the pin array from persisted config (call after ConfigManager::load()).
	// Disabled relays (pin == PinDisabled) will be skipped by all hardware writes.
	void syncPinsFromConfig()
	{
		Config* config = ConfigManager::getConfigPtr();

		if (config == nullptr)
			return;

		for (uint8_t i = 0; i < _relayCount; i++)
		{
			_relays[i] = config->relay.relays[i].pin;
		}
	}
};