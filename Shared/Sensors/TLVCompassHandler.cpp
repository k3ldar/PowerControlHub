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
#include "TLVCompassHandler.h"
#include <Arduino.h>

// Directions array
const char* TLVCompassHandler::directions[16] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
};

TLVCompassHandler::TLVCompassHandler(SerialCommandManager* commandMgrComputer, int filterSize)
	: SingleLoggerSupport(commandMgrComputer),
      sensor(Wire, TLx493D_IIC_ADDR_A0_e),
      filterSize(filterSize > 32 ? 32 : filterSize),
      filterIndex(0), bxSum(0), bySum(0),
      bxFiltered(0), byFiltered(0),
      heading(0), headingSum(0), headingIndex(0),
      pitch(0), roll(0),
      temp(0), bx(0), by(0), bz(0),
      ax(0), ay(0), az(0),
      bxMin(0), bxMax(0), byMin(0), byMax(0), bzMin(0), bzMax(0),
      firstUpdate(true)
{
    for (int i = 0; i < 32; i++) headingBuffer[i] = 0;
}

bool TLVCompassHandler::begin()
{
    Wire.begin();
    Wire.setClock(100000);
    delay(500);

    // Magnetometer
    if (!sensor.begin())
    {
        return false;
    }

#if defined(Mcu_Module)

    // MPU6050
    delay(500);
    if (!mpu.begin())
    {
        return false;
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
#endif

    lastUpdate = millis();

    return true;
}

bool TLVCompassHandler::update(unsigned long now)
{
    double dt = (now - lastUpdate) / 1000.0;
    lastUpdate = now;

    // Magnetometer
    if (!sensor.getTemperature(&temp) || !sensor.getMagneticField(&bx, &by, &bz))
    {
        return false;
    }

    if (firstUpdate)
    {
        bxMin = bxMax = bx;
        byMin = byMax = by;
        bzMin = bzMax = bz;
        firstUpdate = false;
    }
    else
    {
        if (bx < bxMin)
            bxMin = bx; 

        if (bx > bxMax) 
            bxMax = bx;

        if (by < byMin)
            byMin = by;

        if (by > byMax)
            byMax = by;

        if (bz < bzMin)
            bzMin = bz;

        if (bz > bzMax)
            bzMax = bz;
    }

    double bxCal = bx - (bxMin + bxMax) / 2;
    double byCal = by - (byMin + byMax) / 2;
    double bzCal = bz - (bzMin + bzMax) / 2;

    smooth(bxCal, byCal);

#if defined(Mcu_Module)

    // Accelerometer
    sensors_event_t a, g, t;


    mpu.getEvent(&a, &g, &t);

    ax = a.acceleration.x / 9.81; // normalize back to g
    ay = a.acceleration.y / 9.81;
    az = a.acceleration.z / 9.81;

    double ax_ms2_meas = a.acceleration.x;
    double ay_ms2_meas = a.acceleration.y;
    double az_ms2_meas = a.acceleration.z;

    // Calibration
    if (!accelCalibrated)
    {
        accelCalSumX += ax_ms2_meas;
        accelCalSumY += ay_ms2_meas;
        accelCalSumZ += az_ms2_meas;
        accelCalCount++;
        if (accelCalCount >= accelCalSamples)
        {
            accelBiasX = accelCalSumX / accelCalCount;
            accelBiasY = accelCalSumY / accelCalCount;
            accelBiasZ = accelCalSumZ / accelCalCount;
            accelCalibrated = true;
            sendDebug(F("Accel calibration complete."));
        }
        return true;
    }
#else
    double ax_ms2_meas = 0;
    double ay_ms2_meas = 0;
    double az_ms2_meas = 0;
  
#endif

    // Gravity removal
    pitch = atan2(-ax, sqrt(ay*ay + az*az));
    roll  = atan2(ay, az);

    double g_x = -sin(pitch);
    double g_y = sin(roll) * cos(pitch);
    double g_z = cos(roll) * cos(pitch);

    double grav_x_ms2 = g_x * 9.81;
    double grav_y_ms2 = g_y * 9.81;
    double grav_z_ms2 = g_z * 9.81;

    double ax_lin = ax_ms2_meas - grav_x_ms2 - (accelBiasX - grav_x_ms2);
    double ay_lin = ay_ms2_meas - grav_y_ms2 - (accelBiasY - grav_y_ms2);
    double az_lin = az_ms2_meas - grav_z_ms2 - (accelBiasZ - grav_z_ms2);

    if (fabs(ax_lin) < motionThreshold) ax_lin = 0;
    if (fabs(ay_lin) < motionThreshold) ay_lin = 0;
    if (fabs(az_lin) < motionThreshold) az_lin = 0;

    double linNorm = sqrt(ax_lin*ax_lin + ay_lin*ay_lin + az_lin*az_lin);
    if (linNorm == 0.0)
    {
        if (!isStationary)
        {
            if (stationarySince == 0) stationarySince = now;
            else if ((now - stationarySince) >= stationaryTime)
            {
                vx = vy = vz = 0.0;
                isStationary = true;
            }
        }
    }
    else
    {
        stationarySince = 0;
        isStationary = false;
        vx += ax_lin * dt;
        vy += ay_lin * dt;
        vz += az_lin * dt;
    }

    // Heading
    heading = atan2(
        byFiltered * cos(pitch) - bzCal * sin(pitch),
        bxFiltered * cos(roll) + byFiltered * sin(roll) * sin(pitch) + bzCal * sin(roll) * cos(pitch)
    ) * 180.0 / M_PI;
    if (heading < 0) heading += 360.0;
    smoothHeading(heading);

    return true;
}

void TLVCompassHandler::smooth(double newBx, double newBy)
{
    bxSum -= bxFiltered;
    bySum -= byFiltered;

    bxFiltered = newBx;
    byFiltered = newBy;

    bxSum += newBx;
    bySum += newBy;

    filterIndex = (filterIndex + 1) % filterSize;

    bxFiltered = bxSum / filterSize;
    byFiltered = bySum / filterSize;
}

void TLVCompassHandler::smoothHeading(double newHeading)
{
    headingSum -= headingBuffer[headingIndex];
    headingBuffer[headingIndex] = newHeading;
    headingSum += newHeading;

    headingIndex = (headingIndex + 1) % filterSize;

    heading = headingSum / filterSize;
}

// Getters
double TLVCompassHandler::getHeading() const { return heading; }

const char* TLVCompassHandler::getDirection() const {
    int index = (int)((getHeading() + 11.25) / 22.5) % 16;
    return directions[index];
}

double TLVCompassHandler::getTemperature() const { return temp; }
double TLVCompassHandler::getBx() const { return bxFiltered; }
double TLVCompassHandler::getBy() const { return byFiltered; }
double TLVCompassHandler::getBz() const { return bz; }
double TLVCompassHandler::getAx() const { return ax; }
double TLVCompassHandler::getAy() const { return ay; }
double TLVCompassHandler::getAz() const { return az; }
