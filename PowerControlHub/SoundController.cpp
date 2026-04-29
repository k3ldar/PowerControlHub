/*
 * PowerControlHub
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
#include "SoundController.h"
#include "PowerControlHubConstants.h"
#include "SystemFunctions.h"

// Define sound patterns according to COLREGS
// Maneuvering Signals (COLREGS Rule 34 - one-shot, non-repeating)
const uint16_t PatternStarboard[] = {SoundBlastShortMs};
const uint16_t PatternPort[] = {SoundBlastShortMs, SoundBlastShortMs};
const uint16_t PatternAstern[] = {SoundBlastShortMs, SoundBlastShortMs, SoundBlastShortMs};
const uint16_t PatternDanger[] = {SoundBlastShortMs, SoundBlastShortMs, SoundBlastShortMs, SoundBlastShortMs, SoundBlastShortMs};

// Narrow Channel / Overtaking Signals (COLREGS Rule 34 - one-shot)
const uint16_t PatternOvertakeStarboard[] = {SoundBlastLongMs, SoundBlastLongMs, SoundBlastShortMs};
const uint16_t PatternOvertakePort[] = {SoundBlastLongMs, SoundBlastLongMs, SoundBlastShortMs, SoundBlastShortMs};
const uint16_t PatternOvertakeConsent[] = {SoundBlastLongMs, SoundBlastShortMs, SoundBlastLongMs, SoundBlastShortMs};

// Fog Signals (COLREGS Rule 35 - repeating every 2 minutes)
const uint16_t PatternFog[] = {SoundBlastLongMs};

// SOS (International distress - repeating every 10 seconds)
const uint16_t PatternSos[] = {MorseCodeShortMs, MorseCodeShortMs, MorseCodeShortMs, 
								 MorseCodeLongMs, MorseCodeLongMs, MorseCodeLongMs,
								 MorseCodeShortMs, MorseCodeShortMs, MorseCodeShortMs};

// Test pattern
const uint16_t PatternTest[] = {SoundBlastShortMs};

// Pattern lookup table
const SoundPattern SoundPatterns[] = {
	{nullptr, 0, NoRepeat, 0},                                  // None
	{PatternSos, 9, SosRepeatMs, MorseCodeGapMs},               // Sos
	{PatternFog, 1, FogRepeatMs, SoundBlastGapMs},              // Fog
	{PatternStarboard, 1, NoRepeat, SoundBlastGapMs},           // MoveStarboard
	{PatternPort, 2, NoRepeat, SoundBlastGapMs},                // MovePort
	{PatternAstern, 3, NoRepeat, SoundBlastGapMs},              // MoveAstern
	{PatternDanger, 5, NoRepeat, SoundBlastGapMs},              // MoveDanger
	{PatternOvertakeStarboard, 3, NoRepeat, SoundBlastGapMs},   // OvertakeStarboard
	{PatternOvertakePort, 4, NoRepeat, SoundBlastGapMs},        // OvertakePort
	{PatternOvertakeConsent, 4, NoRepeat, SoundBlastGapMs},     // OvertakeConsent
	{PatternDanger, 5, NoRepeat, SoundBlastGapMs},              // OvertakeDanger
	{PatternTest, 1, NoRepeat, SoundBlastGapMs}                 // Test
};

SoundController::SoundController()
	: _isPlaying(false), _soundType(SoundType::None), _state(SoundState::Idle), _soundStartDelay(0),
	_soundRelayIndex(DefaultValue), _currentBlastIndex(0), _stateStartTime(0), _currentPattern(nullptr)
{
	Config* config = ConfigManager::getConfigPtr();

	if (config != nullptr)
	{
		_soundStartDelay = config->sound.startDelayMs;
	}
}

void SoundController::playSound(const SoundType soundType)
{
	// Stop current pattern if any
	if (_isPlaying)
	{
		stopPattern();
	}

	_soundType = soundType;
	
	if (soundType == SoundType::None)
	{
		_isPlaying = false;
		return;
	}

	const SoundPattern* pattern = getPattern(soundType);
	if (pattern && pattern->durations)
	{
		startPattern(pattern);
	}
}

void SoundController::update()
{
	if (!_isPlaying || !_currentPattern)
		return;

	uint64_t currentTime = SystemFunctions::millis64();
	uint64_t elapsed = currentTime - _stateStartTime;

	switch (_state)
	{
		case SoundState::StartDelay:
		{
			// Wait for startup delay to prevent relay/horn clipping
			if (elapsed >= _soundStartDelay)
			{
				// Delay complete, start first blast
				_state = SoundState::BlastOn;
				_stateStartTime = currentTime;
				startSound();
			}
			break;
		}

		case SoundState::BlastOn:
		{
			// Check if current blast duration is complete
			uint16_t blastDuration = _currentPattern->durations[_currentBlastIndex];
			if (elapsed >= blastDuration)
			{
				stopSound();
				
				_currentBlastIndex++;
				
				// Check if pattern is complete
				if (_currentBlastIndex >= _currentPattern->blastCount)
				{
					// Pattern complete - check if repeating
					if (_currentPattern->repeatInterval > 0)
					{
						_state = SoundState::WaitingRepeat;
						_stateStartTime = currentTime;
					}
					else
					{
						// One-shot pattern complete
						stopPattern();
					}
				}
				else
				{
					// Move to gap between blasts
					_state = SoundState::BlastGap;
					_stateStartTime = currentTime;
				}
			}
			break;
		}

		case SoundState::BlastGap:
		{
			// Wait for gap duration
			if (elapsed >= _currentPattern->gapDuration)
			{
				// Start next blast
				startSound();
				_state = SoundState::BlastOn;
				_stateStartTime = currentTime;
			}
			break;
		}

		case SoundState::WaitingRepeat:
		{
			// Wait for repeat interval
			if (elapsed >= _currentPattern->repeatInterval)
			{
				// Restart pattern
				_currentBlastIndex = 0;
				_state = SoundState::BlastOn;
				_stateStartTime = currentTime;
			}
			break;
		}

		default:
			break;
	}
}

void SoundController::startPattern(const SoundPattern* pattern)
{
	_currentPattern = pattern;
	_currentBlastIndex = 0;
	_state = SoundState::StartDelay;
	_stateStartTime = SystemFunctions::millis64();
	_isPlaying = true;
}

void SoundController::stopPattern()
{
	stopSound();
	
	_currentPattern = nullptr;
	_currentBlastIndex = 0;
	_state = SoundState::Idle;
	_stateStartTime = 0;
	_isPlaying = false;
	_soundType = SoundType::None;
}

void SoundController::startSound()
{
	if (_soundRelayIndex != DefaultValue && _soundRelayIndex < ConfigRelayCount)
	{
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr) return;
		uint8_t relayPin = config->relay.relays[_soundRelayIndex].pin;
		if (relayPin != DefaultValue)
		{
			digitalWrite(relayPin, LOW); // Activate relay
		}
	}
}

void SoundController::stopSound()
{
	if (_soundRelayIndex != DefaultValue && _soundRelayIndex < ConfigRelayCount)
	{
		Config* config = ConfigManager::getConfigPtr();
		if (config == nullptr) return;
		uint8_t relayPin = config->relay.relays[_soundRelayIndex].pin;
		if (relayPin != DefaultValue)
		{
			digitalWrite(relayPin, HIGH); // Deactivate relay
		}
	}
}

const SoundPattern* SoundController::getPattern(SoundType soundType) const
{
	uint8_t index = static_cast<uint8_t>(soundType);
	if (index < sizeof(SoundPatterns) / sizeof(SoundPatterns[0]))
	{
		return &SoundPatterns[index];
	}
	return nullptr;
}

void SoundController::configUpdated(Config* config)
{
	if (config != nullptr)
	{
		_soundStartDelay = config->sound.startDelayMs;
		_soundRelayIndex = config->sound.hornRelayIndex;
	}
}