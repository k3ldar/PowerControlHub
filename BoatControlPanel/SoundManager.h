#pragma once

class SoundManager
{
private:
	uint8_t _tonePin;
	unsigned long _lastPlayTime;
	unsigned long _nextPlayTime;
	uint16_t _duration;
	uint16_t _toneHz;
	uint16_t _repeatInterval;        
	bool _isPlaying;
	bool _shouldRepeat;

public:
	SoundManager(uint8_t tonePin)
		: _tonePin(tonePin),
		  _lastPlayTime(0),
		  _nextPlayTime(0),
		  _duration(0),
		  _toneHz(0),
		  _repeatInterval(0),
		  _isPlaying(false),
		  _shouldRepeat(false)
	{
		pinMode(_tonePin, OUTPUT);
	}
	
	void playSound(uint16_t duration, uint16_t toneHz, uint16_t repeatInterval = 0)
	{
		_toneHz = toneHz;
		_duration = duration;
		_repeatInterval = repeatInterval;
		_shouldRepeat = (repeatInterval > 0);
		_isPlaying = true;

		tone(_tonePin, toneHz, duration);
		_lastPlayTime = millis();

		if (_shouldRepeat)
		{
			_nextPlayTime = _lastPlayTime + _repeatInterval;
		}
	}

	void stopSound()
	{
		_isPlaying = false;
		_shouldRepeat = false;
		noTone(_tonePin);
	}

	void update(unsigned long now)
	{
		if (_isPlaying && _shouldRepeat && (now >= _nextPlayTime))
		{
			tone(_tonePin, _toneHz, _duration);
			_lastPlayTime = now;
			_nextPlayTime = now + _repeatInterval;
		}
	}
};