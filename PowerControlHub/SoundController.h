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

#include "ConfigManager.h"

struct SoundPattern
{
	const uint16_t* durations;  // Array of blast durations in ms
	uint8_t blastCount;         // Number of blasts in the pattern
	uint32_t repeatInterval;    // 0 = no repeat, else ms between repetitions
	uint16_t gapDuration;       // Gap between blasts in ms
};

enum class SoundState : uint8_t
{
	Idle,
	StartDelay,
	BlastOn,
	BlastGap,
	WaitingRepeat
};

enum class SoundType : uint8_t
{
	None = 0x00,
	Sos = 0x01,
	Fog = 0x02,
	MoveStarboard = 0x03,
	MovePort = 0x04,
	MoveAstern = 0x05,
	MoveDanger = 0x06,
	OvertakeStarboard = 0x07,
	OvertakePort = 0x08,
	OvertakeConsent = 0x09,
	OvertakeDanger = 0x0A,
	Test = 0x0B
};

class SoundController
{
private:
	bool _isPlaying;
	SoundType _soundType;
	SoundState _state;
	uint16_t _soundStartDelay;
	uint8_t _soundRelayIndex;
	
	uint8_t _currentBlastIndex;
	uint64_t _stateStartTime;
	
	const SoundPattern* _currentPattern;
	
	void startPattern(const SoundPattern* pattern);
	void stopPattern();
	const SoundPattern* getPattern(SoundType soundType) const;
	void startSound();
	void stopSound();

public:
	SoundController();
	void playSound(const SoundType soundType);
	bool isPlaying() const { return _isPlaying; }
	void update();
	SoundType getCurrentSoundType() const { return _soundType; }
	SoundState getCurrentSoundState() const { return _state; }
	void configUpdated(Config* config);
};
