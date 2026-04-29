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
#include "ToneManager.h"
#include "SystemFunctions.h"

ToneManager::ToneManager(uint8_t pin)
    : _pin(pin),
      _config(nullptr),
      _playing(false),
      _currentStep(0),
      _totalSteps(0),
      _stepStartTime(0)
{
    pinMode(_pin, OUTPUT);
}

void ToneManager::configSet(SoundConfig* config)
{
    _config = config;
}

void ToneManager::play(ToneType type)
{
    stop();
    buildSequence(type);

    if (_totalSteps > 0)
    {
        _playing = true;
        _currentStep = 0;
        _stepStartTime = SystemFunctions::millis64();
        startCurrentStep();
    }
}

void ToneManager::stop()
{
    if (_playing)
    {
        noTone(_pin);
    }

    _playing = false;
    _currentStep = 0;
    _totalSteps = 0;
}

bool ToneManager::isPlaying() const
{
    return _playing;
}

uint32_t ToneManager::getRepeatIntervalMs() const
{
    return _config ? _config->badRepeatMs : 30000;
}

void ToneManager::update(uint64_t now)
{
    if (!_playing)
        return;

    if (SystemFunctions::hasElapsed(now, _stepStartTime, _steps[_currentStep].durationMs))
    {
        _currentStep++;

        if (_currentStep >= _totalSteps)
        {
            stop();
            return;
        }

        _stepStartTime = now;
        startCurrentStep();
    }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void ToneManager::startCurrentStep()
{
    const ToneStep& step = _steps[_currentStep];


    if (step.frequencyHz > 0)
    {
        tone(_pin, step.frequencyHz, step.durationMs);
    }
    else
    {
        noTone(_pin);
    }
}

void ToneManager::buildSequence(ToneType type)
{
    uint8_t preset;
    uint16_t hz;
    uint16_t ms;

    if (type == ToneType::Good)
    {
        preset = _config ? _config->goodPreset : 0;
        hz     = _config ? _config->goodToneHz : 1000;
        ms     = _config ? _config->goodDurationMs : 100;
    }
    else
    {
        preset = _config ? _config->badPreset : 0;
        hz     = _config ? _config->badToneHz : 500;
        ms     = _config ? _config->badDurationMs : 200;
    }

	switch (static_cast<TonePreset>(preset))
	{
		case TonePreset::SubmarinePing:
			_totalSteps = buildSubmarinePing();
			break;

		case TonePreset::DoubleBeep:
			_totalSteps = buildDoubleBeep();
			break;

		case TonePreset::RisingChirp:      
			_totalSteps = buildRisingChirp();       
			break;

		case TonePreset::DescendingAlert:  
			_totalSteps = buildDescendingAlert();   
			break;

		case TonePreset::NauticalBell:     
			_totalSteps = buildNauticalBell();      
			break;

		case TonePreset::UserDefined:
			_totalSteps = buildUserDefined(hz, ms);
			break;

		case TonePreset::NoSound:
		default:                           
			_totalSteps = 0;
			break;

	}
}

// ---------------------------------------------------------------------------
// Preset builders
// ---------------------------------------------------------------------------

uint8_t ToneManager::buildUserDefined(uint16_t hz, uint16_t ms)
{
    _steps[0] = { hz, ms };
    return 1;
}

// Sonar-style sweep 1000→400 Hz then fading echo pips
uint8_t ToneManager::buildSubmarinePing()
{
    uint8_t i = 0;
    constexpr uint16_t startF = 1000;
    constexpr uint16_t endF   = 400;
    constexpr uint8_t  sweepSteps = 8;
    constexpr uint16_t stepDur = 6; // ms per sweep slice (~48 ms total)

    for (uint8_t s = 0; s <= sweepSteps && i < MaxSteps; ++s)
    {
        uint16_t f = startF + (int16_t)(endF - startF) * s / sweepSteps;
        _steps[i++] = { f, stepDur };
    }

    if (i < MaxSteps) _steps[i++] = { 0,   30 }; // pause
    if (i < MaxSteps) _steps[i++] = { 600, 30 }; // echo 1
    if (i < MaxSteps) _steps[i++] = { 0,   30 }; // pause
    if (i < MaxSteps) _steps[i++] = { 500, 25 }; // echo 2
    if (i < MaxSteps) _steps[i++] = { 0,   45 }; // pause
    if (i < MaxSteps) _steps[i++] = { 450, 20 }; // echo 3

    return i;
}

// Two quick identical pips
uint8_t ToneManager::buildDoubleBeep()
{
    _steps[0] = { 1200, 80 };
    _steps[1] = { 0,    80 };
    _steps[2] = { 1200, 80 };
    return 3;
}

// Four ascending tones
uint8_t ToneManager::buildRisingChirp()
{
    _steps[0] = { 400,  60 };
    _steps[1] = { 600,  60 };
    _steps[2] = { 800,  60 };
    _steps[3] = { 1000, 80 };
    return 4;
}

// Three descending tones
uint8_t ToneManager::buildDescendingAlert()
{
    _steps[0] = { 1200, 80 };
    _steps[1] = { 900,  80 };
    _steps[2] = { 600, 100 };
    return 3;
}

// Quick double ding (high-pitched)
uint8_t ToneManager::buildNauticalBell()
{
    _steps[0] = { 2000, 40 };
    _steps[1] = { 0,    20 };
    _steps[2] = { 2000, 40 };
    return 3;
}
