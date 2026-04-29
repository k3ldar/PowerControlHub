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

class RgbLedFade {
private:
    uint8_t _rPin;
    uint8_t _gPin;
    uint8_t _bPin;

    uint8_t baseR = 0;
    uint8_t baseG = 0;
    uint8_t baseB = 0;

    float level = 0.0f;
    float fadeStep = 0.01f;
    int direction = 1;

    unsigned long lastUpdate = 0;
    const unsigned long updateInterval = 10; // ms

    uint8_t _maxBrightness = 255;
    uint8_t _minBrightness = 0;

    Config* _config = nullptr;
    bool _isDayTime = true;
    bool _isWarning = false;

    /**
     * @brief Apply configuration based on current day/night and warning state
     */
    void applyConfigColors()
    {
        if (!_config) return;

        const LedConfig& led = _config->led;
        
        // Select color based on day/night and warning state
        const uint8_t* color;
        uint8_t brightness;

        if (_isDayTime)
        {
            color = _isWarning ? led.dayBadColor : led.dayGoodColor;
            brightness = led.dayBrightness;
        }
        else
        {
            color = _isWarning ? led.nightBadColor : led.nightGoodColor;
            brightness = led.nightBrightness;
        }

        // Apply color
        setColor(color[0], color[1], color[2]);
        
        // Apply brightness (convert 0-100 to 0-255)
        setMaxBrightness((brightness * 255) / 100);
    }

public:
    RgbLedFade(uint8_t rPin, uint8_t gPin, uint8_t bPin)
        : _rPin(rPin), _gPin(gPin), _bPin(bPin)
    {
    }

    void begin()
    {
        pinMode(_rPin, OUTPUT);
        pinMode(_gPin, OUTPUT);
        pinMode(_bPin, OUTPUT);
        
        // Initialize to off
        analogWrite(_rPin, 0);
        analogWrite(_gPin, 0);
        analogWrite(_bPin, 0);
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b)
    {
        baseR = r;
        baseG = g;
        baseB = b;
    }

    void setFadeSpeed(float step)
    {
        fadeStep = step;
    }

    void setMaxBrightness(uint8_t brightness)
    {
        _maxBrightness = brightness;
    }

    void setMinBrightness(uint8_t brightness)
    {
        _minBrightness = brightness;
    }

    void setDayTime(bool isDaytime)
    {
        if (_isDayTime != isDaytime)
        {
            _isDayTime = isDaytime;
            applyConfigColors();
        }
    }

    void setWarning(bool isWarning)
    {
        if (_isWarning != isWarning)
        {
            _isWarning = isWarning;
            applyConfigColors();
        }
    }

    void configSet(Config* config)
    {
        _config = config;
        applyConfigColors();
    }

    void refreshFromConfig()
    {
        applyConfigColors();
    }

    void update(unsigned long now)
    {    
        if (now - lastUpdate < updateInterval) return;
        lastUpdate = now;

        level += direction * fadeStep;
        if (level >= 1.0f)
        {
            level = 1.0f;
            direction = -1;
        }
        else if (level <= 0.0f)
        {
            level = 0.0f;
            direction = 1;
        }

        float gamma = pow(level, 2.2f);
        float brightnessFactor = gamma * (_maxBrightness / 255.0f);

        if (brightnessFactor < (_minBrightness / 255.0f))
        {
            brightnessFactor = _minBrightness / 255.0f;
        }

        analogWrite(_rPin, (uint8_t)(baseR * brightnessFactor));
        analogWrite(_gPin, (uint8_t)(baseG * brightnessFactor));
        analogWrite(_bPin, (uint8_t)(baseB * brightnessFactor));
    }
};
