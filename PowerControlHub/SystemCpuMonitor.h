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

// Default to 1 second, but can be changed
#define CPU_MONITOR_WINDOW_MS 1000

class SystemCpuMonitor
{
private:
    static uint64_t _busyTimeMicros;
    static uint64_t _windowStartMillis;
    static uint64_t _taskStartMicros;
    static uint8_t _lastCpuPercent;
    static bool _taskTiming;
    static uint64_t _windowSizeMs;

public:
    // Start timing a task
    static void startTask();
    
    // End timing a task
    static void endTask();
    
    // Get current CPU usage percentage (0-100)
    static uint8_t getCpuUsage();
    
    // Call periodically to update measurements
    static void update();
    
    // Set the measurement window size in milliseconds (default: 1000ms)
    // Smaller values = more responsive but noisier
    // Larger values = more stable but less responsive
    static void setWindowSize(uint64_t milliseconds);
};

