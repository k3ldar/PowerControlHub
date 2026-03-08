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
#pragma once

#define _Mcu_Module

#include <SerialCommandManager.h>

#include "LoggingSupport.h"
#include <Wire.h>
#include <math.h>

#if defined(Mcu_Module)
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#endif

#include "TLx493D_inc.hpp"

using namespace ifx::tlx493d;


class TLVCompassHandler : public SingleLoggerSupport
{
public:
    TLVCompassHandler(SerialCommandManager* commandMgrComputer, int filterSize = 5);

    bool begin();
    bool update(unsigned long now);

    double getHeading() const;          // Tilt-compensated heading in degrees
    const char* getDirection() const;   // N, NNE, NE, etc.
    double getTemperature() const;
    double getBx() const;
    double getBy() const;
    double getBz() const;
    double getAx() const;
    double getAy() const;
    double getAz() const;

    // Velocity
    double getVx() const { return vx; }
    double getVy() const { return vy; }
    double getVz() const { return vz; }

private:
    static const char* directions[16];

    // Magnetometer
    TLx493D_A1B6 sensor;

#if defined(Mcu_Module)
    // Accelerometer/Gyro
    Adafruit_MPU6050 mpu;
#endif

    // Smoothing
    int filterSize;
    int filterIndex;
    double bxSum, bySum;
    double bxFiltered, byFiltered;
    double heading;
    double headingSum;
    int headingIndex;
    double headingBuffer[32];
    double pitch, roll;

    // Raw readings
    double temp, bx, by, bz;
    double ax, ay, az;

    // Auto-calibration
    double bxMin, bxMax;
    double byMin, byMax;
    double bzMin, bzMax;
    bool firstUpdate;

    // Accel calibration
    double accelBiasX = 0.0;
    double accelBiasY = 0.0;
    double accelBiasZ = 0.0;
    bool accelCalibrated = false;
    int accelCalSamples = 200;
    int accelCalCount = 0;
    double accelCalSumX = 0;
    double accelCalSumY = 0;
    double accelCalSumZ = 0;

    // Velocity & ZUPT
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    unsigned long lastUpdate = 0;
    const double motionThreshold = 0.1;
    unsigned long stationaryTime = 500;
    unsigned long stationarySince = 0;
    bool isStationary = false;

    void smooth(double newBx, double newBy);
    void smoothHeading(double newHeading);
};
