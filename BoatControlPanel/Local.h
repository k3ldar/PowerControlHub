#pragma once
#include <Arduino.h>

#define BOAT_CONTROL_PANEL
#define ARDUINO_MEGA2560
//#define ARDUINO_R4_MINIMA

// Uncomment the following line to enable the screen demo command handler, which cycles through 
// all pages with a specified interval (default 6 seconds), this is a blocking operation and 
// should only be used for testing/demo purposes.
//#define SCREEN_DEMO_SUPPORT


constexpr uint8_t ConfigRelayCount = 8;
