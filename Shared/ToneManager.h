#pragma once
#include <Arduino.h>
#include "Config.h"

// Preset identifiers (shared for good and bad)
enum class TonePreset : uint8_t
{
    UserDefined     = 0, // Uses toneHz / durationMs from SoundSignalConfig
    SubmarinePing   = 1, // Sonar-style sweep + echoes
    DoubleBeep      = 2, // Two short pips
    RisingChirp     = 3, // Ascending four-tone
    DescendingAlert = 4, // Descending three-tone
    NauticalBell    = 5, // Quick double ding
    PresetCount
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

    void configSet(SoundSignalConfig* config);

    // Start playing a good or bad tone sequence (non-blocking)
    void play(ToneType type);

    // Immediately stop any active playback
    void stop();

    bool isPlaying() const;

    // Get the configured repeat interval for bad tones (ms)
    uint32_t getRepeatIntervalMs() const;

    // Call from loop(); advances the step sequence
    void update(unsigned long now);

private:
    static constexpr uint8_t MaxSteps = 16;

    uint8_t _pin;
    SoundSignalConfig* _config;

    bool _playing;
    uint8_t _currentStep;
    uint8_t _totalSteps;
    unsigned long _stepStartTime;
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
