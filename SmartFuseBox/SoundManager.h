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

class SoundManager
{
private:
	bool _isPlaying;
	SoundType _soundType;
	SoundState _state;
	uint16_t _soundStartDelay;
	uint8_t _soundRelayIndex;
	
	uint8_t _currentBlastIndex;
	unsigned long _stateStartTime;
	
	const SoundPattern* _currentPattern;
	
	void startPattern(const SoundPattern* pattern);
	void stopPattern();
	const SoundPattern* getPattern(SoundType soundType) const;
	void startSound();
	void stopSound();

public:
	SoundManager();
	void playSound(const SoundType soundType);
	bool isPlaying() const { return _isPlaying; }
	void update();
	SoundType getCurrentSoundType() const { return _soundType; }
	SoundState getCurrentSoundState() const { return _state; }
	void configUpdated(Config* config);
};
