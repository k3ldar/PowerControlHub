#pragma once

#include <Arduino.h>

// Default to 1 second, but can be changed
#define CPU_MONITOR_WINDOW_MS 1000

class SystemCpuMonitor
{
private:
    static unsigned long _busyTimeMicros;
    static unsigned long _windowStartMillis;
    static unsigned long _taskStartMicros;
    static uint8_t _lastCpuPercent;
    static bool _taskTiming;
    static unsigned long _windowSizeMs;

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
    static void setWindowSize(unsigned long milliseconds);
};

