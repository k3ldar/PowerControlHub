#pragma once

#include "SystemDefinitions.h"

enum class RelayResult : uint8_t
{
	Success = 0,
	InvalidIndex = 1,
	Reserved = 2
};


class RelayController
{
private:
	bool* _relayStatus;
	uint8_t* _relays;
	uint8_t _relayCount;
	uint8_t _reservedSoundRelay;

	RelayResult setRelayStatus(uint8_t relayIndex, bool isOn)
	{
		if (relayIndex >= _relayCount)
		{
			return RelayResult::InvalidIndex;
		}

		if (relayIndex == _reservedSoundRelay)
		{
			return RelayResult::Reserved;
		}

		_relayStatus[relayIndex] = isOn;
		digitalWrite(_relays[relayIndex], isOn ? LOW : HIGH);

		return RelayResult::Success;
	}

public:
	RelayController(const uint8_t* relayPins, uint8_t totalRelays)
		: _relayStatus(nullptr), _relays(nullptr), _relayCount(totalRelays), _reservedSoundRelay(DefaultValue)
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
			pinMode(_relays[i], OUTPUT);
			digitalWrite(_relays[i], HIGH);
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
		uint8_t status = 0xFF;
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

	uint8_t getReservedSoundRelay() const { return _reservedSoundRelay; }

	void setReservedSoundRelay(uint8_t relayIndex) { _reservedSoundRelay = relayIndex; }
};