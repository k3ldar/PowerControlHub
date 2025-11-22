#pragma once
#include <Arduino.h>
#include "Local.h"

class SharedFunctions {
public:
    // Call once at startup (handles any board-specific init)
    static uint16_t freeMemory();
};