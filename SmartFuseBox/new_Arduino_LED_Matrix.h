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
// after having some serious led issues within R4 Uno Wifi I copied the below code from:
// https://github.com/KurtE/UNOR4-stuff/blob/527daeb54ae59fbc76b9c9b9b70efd422ca92754/test_sketches/matrix_multi_leds_on_test/new_Arduino_LED_Matrix.h
// so far this works much better.

#pragma once
#include "Arduino.h"
#include "FspTimer.h"

#define NUM_LEDS    96

static const int pin_zero_index = 28;

static const uint8_t pins[][2] = {
  { 7, 3 }, // 0
  { 3, 7 },
  { 7, 4 },
  { 4, 7 },
  { 3, 4 },
  { 4, 3 },
  { 7, 8 },
  { 8, 7 },
  { 3, 8 },
  { 8, 3 },
  { 4, 8 }, // 10
  { 8, 4 },
  { 7, 0 },
  { 0, 7 },
  { 3, 0 },
  { 0, 3 },
  { 4, 0 },
  { 0, 4 },
  { 8, 0 },
  { 0, 8 },
  { 7, 6 }, // 20
  { 6, 7 },
  { 3, 6 },
  { 6, 3 },
  { 4, 6 },
  { 6, 4 },
  { 8, 6 },
  { 6, 8 },
  { 0, 6 },
  { 6, 0 },
  { 7, 5 }, // 30
  { 5, 7 },
  { 3, 5 },
  { 5, 3 },
  { 4, 5 },
  { 5, 4 },
  { 8, 5 },
  { 5, 8 },
  { 0, 5 },
  { 5, 0 },
  { 6, 5 }, // 40
  { 5, 6 },
  { 7, 1 },
  { 1, 7 },
  { 3, 1 },
  { 1, 3 },
  { 4, 1 },
  { 1, 4 },
  { 8, 1 },
  { 1, 8 },
  { 0, 1 }, // 50
  { 1, 0 },
  { 6, 1 },
  { 1, 6 },
  { 5, 1 },
  { 1, 5 },
  { 7, 2 },
  { 2, 7 },
  { 3, 2 },
  { 2, 3 },
  { 4, 2 },
  { 2, 4 },
  { 8, 2 },
  { 2, 8 },
  { 0, 2 },
  { 2, 0 },
  { 6, 2 },
  { 2, 6 },
  { 5, 2 },
  { 2, 5 },
  { 1, 2 },
  { 2, 1 },
  { 7, 10 },
  { 10, 7 },
  { 3, 10 },
  { 10, 3 },
  { 4, 10 },
  { 10, 4 },
  { 8, 10 },
  { 10, 8 },
  { 0, 10 },
  { 10, 0 },
  { 6, 10 },
  { 10, 6 },
  { 5, 10 },
  { 10, 5 },
  { 1, 10 },
  { 10, 1 },
  { 2, 10 },
  { 10, 2 },
  { 7, 9 },
  { 9, 7 },
  { 3, 9 },
  { 9, 3 },
  { 4, 9 },
  { 9, 4 },
};

static uint32_t volatile * const matrix_pin_pmnpfs[] = {
     &R_PFS->PORT[(g_pin_cfg[0 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[0 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[1 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[1 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[2 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[2 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[3 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[3 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[4 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[4 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[5 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[5 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[6 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[6 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[7 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[7 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[8 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[8 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[9 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[9 + pin_zero_index].pin) & 0xFF].PmnPFS,
     &R_PFS->PORT[(g_pin_cfg[10 + pin_zero_index].pin) >> 8].PIN[(g_pin_cfg[10 + pin_zero_index].pin) & 0xFF].PmnPFS
};


/*static*/ const int8_t matrix_irq_pins[11][10] = {
{50/*01*/, 64/*02*/, 15/*03*/, 17/*04*/, 38/*05*/ , 28/*06*/, 13/*07*/, 19/*08*/, 80/*10*/, -1},    // high 0
{51/*00*/, 70/*02*/, 45/*03*/, 47/*04*/, 55/*05*/, 53/*06*/, 43/*07*/, 49/*08*/, 86/*10*/, -1},     // High 1
{65/*00*/, 71/*01*/, 59/*03*/, 61/*04*/, 69/*05*/, 67/*06*/, 57/*07*/, 63/*08*/, 88/*10*/, -1},     // High 2
{14/*00*/, 44/*01*/, 58/*02*/, 4/*04*/, 32/*05*/ , 22/*06*/, 1/*07*/, 8/*08*/, 92/*09*/, 74/*10*/}, // High 3
{16/*00*/, 46/*01*/, 60/*02*/, 5/*03*/, 34/*05*/ , 24/*06*/, 3/*07*/, 10/*08*/, 94/*09*/, 76/*10*/},// High 4
{39/*00*/ , 54/*01*/, 68/*02*/, 33/*03*/ , 35/*04*/ , 41/*06*/, 31/*07*/, 37/*08*/ , 84/*10*/, -1}, // High 5
{29/*00*/, 52/*01*/, 66/*02*/, 23/*03*/, 25/*04*/, 40/*05*/, 21/*07*/, 27/*08*/, 82/*10*/, -1},     // High 6
{12/*00*/, 42/*01*/, 56/*02*/, 0/*03*/, 2/*04*/, 30/*05*/, 20/*06*/, 6/*08*/, 90/*09*/, 72/*10*/},  // High 7
{18/*00*/, 48/*01*/, 62/*02*/, 9/*03*/, 11/*04*/, 36/*05*/ , 26/*06*/, 7/*07*/, 78/*10*/, -1},      // High 8
{93/*03*/, 95/*04*/, 91/*07*/, -1},                                                                 // High 9
{81/*00*/, 87/*01*/, 89/*02*/, 75/*03*/, 77/*04*/, 85/*05*/, 83/*06*/, 73/*07*/, 79/*08*/, -1},     // High 10

};

#define LED_MATRIX_PORT0_MASK       ((1 << 3) | (1 << 4) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 15))
#define LED_MATRIX_PORT2_MASK       ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 12) | (1 << 13))


static uint32_t reverse(uint32_t x)
{
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
    x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
    x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
    return x;
}

// TODO: this is dangerous, use with care
#define loadSequence(frames)                loadWrapper(frames, sizeof(frames))
#define renderBitmap(bitmap, rows, columns) loadPixels(&bitmap[0][0], rows*columns)

static uint8_t __attribute__((aligned)) framebuffer[NUM_LEDS / 8];

class ArduinoLEDMatrix {

public:
    ArduinoLEDMatrix() {}
    // TODO: find a better name
    // autoscroll will be slower than calling next() at precise times
    void autoscroll(uint32_t interval_ms) {
        _interval = interval_ms;
    }
    void on(size_t pin, bool clear = true ) {
      if (clear) {
        R_PORT0->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT0_MASK);
        R_PORT2->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT2_MASK);
      }
      *matrix_pin_pmnpfs[pins[pin][0]] = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH;
      *matrix_pin_pmnpfs[pins[pin][1]] = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW;
    }

    void off(__attribute__((unused)) size_t pin=0) {
      R_PORT0->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT0_MASK);
      R_PORT2->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT2_MASK);
    }

    void begin() {
        uint8_t type;
        uint8_t ch = FspTimer::get_available_timer(type);
        // TODO: avoid passing "this" argument to remove autoscroll
        _ledTimer.begin(TIMER_MODE_PERIODIC, type, ch, 10000.0, 50.0, turnOnMultiLedISR, this);
        _ledTimer.setup_overflow_irq();
        _ledTimer.open();
        _ledTimer.start();
    }
    void next() {
        uint32_t frame[3];
        frame[0] = reverse(*(_frames+(_currentFrame*4)+0));
        frame[1] = reverse(*(_frames+(_currentFrame*4)+1));
        frame[2] = reverse(*(_frames+(_currentFrame*4)+2));
        _interval = *(_frames+(_currentFrame*4)+3);
        _currentFrame = (_currentFrame + 1) % _framesCount;
        if(_currentFrame == 0){
            if(!_loop){
                _interval = 0;
            }
            if(_callBack != nullptr){
                _callBack();
                _sequenceDone = true;
            }
        }
        memcpy(framebuffer, (uint32_t*)frame, sizeof(frame));
    }
    void loadFrame(const uint32_t buffer[3]){
        uint32_t tempBuffer[][4] = {{
            buffer[0], buffer[1], buffer[2], 0
        }};
        loadSequence(tempBuffer);
        next();
        _interval = 0;
    }
    void renderFrame(uint8_t frameNumber){
        _currentFrame = frameNumber % _framesCount;
        next();
        _interval = 0;
    }
    void play(bool loop = false){
        _loop = loop;
        _sequenceDone = false;
        next();
    }
    bool sequenceDone(){
        if(_sequenceDone){
            _sequenceDone = false;
            return true;
        }
        return false;
    }

    void loadPixels(uint8_t *arr, size_t size){
        uint32_t partialBuffer = 0;
        uint8_t pixelIndex = 0;
        uint8_t *frameP = arr;
        uint32_t *frameHolderP = _frameHolder;
        while (pixelIndex < size) {
            partialBuffer |= *frameP++;
            if ((pixelIndex + 1) % 32 == 0) {
                *(frameHolderP++) = partialBuffer;
            }
            partialBuffer = partialBuffer << 1;
            pixelIndex++;
        }
        loadFrame(_frameHolder);
    };

    void loadWrapper(const uint32_t frames[][4], uint32_t howMany) {
        _currentFrame = 0;
        _frames = (uint32_t*)frames;
        _framesCount = (howMany / 4) / sizeof(uint32_t);
    }
    // WARNING: callbacks are fired from ISR. The execution time will be limited.
    void setCallback(voidFuncPtr callBack){
        _callBack = callBack;
    }
    
private:
    int _currentFrame = 0;
    uint32_t _frameHolder[3];
    uint32_t* _frames;
    uint32_t _framesCount;
    uint32_t _interval = 0;
    uint32_t _lastInterval = 0;
    bool _loop = false;
    FspTimer _ledTimer;
    bool _sequenceDone = false;
    voidFuncPtr _callBack;

    static void turnOnMultiLedISR(timer_callback_args_t *arg) {
        static volatile int i_isr = 0;
        // turn all of the pins to input.
        R_PORT0->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT0_MASK);
        R_PORT2->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT2_MASK);
        bool first_on = true;
        for (uint8_t i = 0; i < 10; i++) {
          uint8_t led = matrix_irq_pins[i_isr][i];
          if (led == (uint8_t)-1) break;
          if ((framebuffer[led >> 3] & (1 << (led % 8))) != 0) {
            if (first_on) {
              *matrix_pin_pmnpfs[i_isr] = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH;
              first_on = false;
            }
            *matrix_pin_pmnpfs[pins[led][1]] = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW;
          }
        }
        i_isr = (i_isr + 1) % 11;
        if (arg != nullptr && arg->p_context != nullptr) {
            ArduinoLEDMatrix* _m = (ArduinoLEDMatrix*)arg->p_context;
            if (_m->_interval != 0 && millis() - _m->_lastInterval > _m->_interval) {
                _m->next();
                _m->_lastInterval = millis();
            }
        }
    }
};
