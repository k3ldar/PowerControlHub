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
#include "SystemCpuMonitor.h"


unsigned long SystemCpuMonitor::_busyTimeMicros = 0;
unsigned long SystemCpuMonitor::_windowStartMillis = 0;
unsigned long SystemCpuMonitor::_taskStartMicros = 0;
uint8_t SystemCpuMonitor::_lastCpuPercent = 0;
bool SystemCpuMonitor::_taskTiming = false;
unsigned long SystemCpuMonitor::_windowSizeMs = CPU_MONITOR_WINDOW_MS;

constexpr uint16_t MinimumWindowMonitorTime = 100;
constexpr uint16_t MaximumWindowMonitorTime = 2000;
constexpr unsigned long MilliSecondsPerSeconds = 1000;
constexpr uint8_t MaxCpuPercentage = 100;
constexpr uint8_t PercentageMultiplier = 100;


void SystemCpuMonitor::startTask()
{
    if (!_taskTiming)
    {
        _taskStartMicros = micros();
        _taskTiming = true;
    }
}

void SystemCpuMonitor::endTask()
{
    if (_taskTiming)
    {
        unsigned long elapsed = micros() - _taskStartMicros;
        _busyTimeMicros += elapsed;
        _taskTiming = false;
    }
}

uint8_t SystemCpuMonitor::getCpuUsage()
{
    return _lastCpuPercent;
}

void SystemCpuMonitor::update()
{
    unsigned long now = millis();
    
    // Initialize on first call
    if (_windowStartMillis == 0)
    {
        _windowStartMillis = now;
        return;
    }
    
    // Calculate when window expires
    unsigned long elapsed = now - _windowStartMillis;
    if (elapsed >= _windowSizeMs)
    {
        // Convert window size to microseconds for calculation
        unsigned long windowMicros = _windowSizeMs * MilliSecondsPerSeconds;
        
        // CPU % = (busy time / total time) * 100
        _lastCpuPercent = min(MaxCpuPercentage, (uint8_t)((_busyTimeMicros * PercentageMultiplier) / windowMicros));

        // Reset for next window
        _busyTimeMicros = 0;
        _windowStartMillis = now;
    }
}

void SystemCpuMonitor::setWindowSize(unsigned long milliseconds)
{
    // Enforce reasonable limits: 100ms to 2 seconds
    if (milliseconds < MinimumWindowMonitorTime)
		milliseconds = MinimumWindowMonitorTime;
	
    if (milliseconds > MaximumWindowMonitorTime)
		milliseconds = MaximumWindowMonitorTime;
    
    _windowSizeMs = milliseconds;
}