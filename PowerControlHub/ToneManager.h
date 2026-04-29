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
#pragma once
#include <Arduino.h>
#include "Config.h"

// Preset identifiers (shared for good and bad)
enum class TonePreset : uint8_t
{
    UserDefined = 0, // Uses toneHz / durationMs from SoundSignalConfig
    SubmarinePing = 1, // Sonar-style sweep + echoes
    DoubleBeep = 2, // Two short pips
    RisingChirp = 3, // Ascending four-tone
    DescendingAlert = 4, // Descending three-tone
    NauticalBell = 5, // Quick double ding
    NoSound = 0xFF, // Special value to indicate silence (ignores toneHz/durationMs)
};

enum class ToneType : uint8_t
{
    Good = 0,
    Bad  = 1
};

// A single step in a tone sequence
struct ToneStep
{
    uint16_t frequencyHz; // 0 = silence / pause
    uint16_t durationMs;
};

class ToneManager
{
public:
    explicit ToneManager(uint8_t pin);

    void configSet(SoundConfig* config);

    // Start playing a good or bad tone sequence (non-blocking)
    void play(ToneType type);

    // Immediately stop any active playback
    void stop();

    bool isPlaying() const;

    // Get the configured repeat interval for bad tones (ms)
    uint32_t getRepeatIntervalMs() const;

    // Call from loop(); advances the step sequence
    void update(uint64_t now);

private:
    static constexpr uint8_t MaxSteps = 16;

    uint8_t _pin;
    SoundConfig* _config;

    bool _playing;
    uint8_t _currentStep;
    uint8_t _totalSteps;
    uint64_t _stepStartTime;
    ToneStep _steps[MaxSteps];

    void buildSequence(ToneType type);
    void startCurrentStep();

    // Preset builders – each returns step count
    uint8_t buildUserDefined(uint16_t hz, uint16_t ms);
    uint8_t buildSubmarinePing();
    uint8_t buildDoubleBeep();
    uint8_t buildRisingChirp();
    uint8_t buildDescendingAlert();
    uint8_t buildNauticalBell();
};
