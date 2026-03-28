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

#include <Arduino.h>
#include "SerialCommandManager.h"
#include "Local.h"

#if defined(FUSE_BOX_CONTROLLER)
#include "RemoteSensor.h"
#include "SharedBaseCommandHandler.h"
#endif

#if defined(BOAT_CONTROL_PANEL)
#include "HomePage.h"
#include "BaseBoatCommandHandler.h"
#include "BaseBoatPage.h"
#endif

// Define the base class type based on the build configuration
#if defined(BOAT_CONTROL_PANEL)
    #define SENSOR_BASE_CLASS BaseBoatCommandHandler
#elif defined(FUSE_BOX_CONTROLLER)
    #define SENSOR_BASE_CLASS SharedBaseCommandHandler
#else
    #error "Either BOAT_CONTROL_PANEL or FUSE_BOX_CONTROLLER must be defined"
#endif

class SensorCommandHandler : public SENSOR_BASE_CLASS
{
private:
	float _lastTemperature = NAN;
	uint8_t _lastHumidity = 0;
	float _lastBearing = NAN;
	float _lastCompassTemp = 0;
	uint8_t _lastSpeed = 0;
	uint16_t _lastWaterLevel = 0;
	bool _lastWaterPumpActive = false;
	bool _isDaytime = true;
	double _gpsLatitude;
	double _gpsLongitude;
	double _altitude;
	double _gpsCourse;
	uint32_t _gpsSatellites;
	const char* _gpsDirection;
	double _gpsDistance = 0;
    bool _lastHornActive = false;
#if defined(FUSE_BOX_CONTROLLER)
	RemoteSensor** _remoteSensors;
	size_t _remoteSensorCount;
#endif

public:
#if defined(BOAT_CONTROL_PANEL)
    explicit SensorCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
	explicit SensorCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);
#endif

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

#if defined(FUSE_BOX_CONTROLLER)
	void setup(RemoteSensor* remoteSensors[], size_t remoteSensorCount);
#endif


	// Getters
	float getTemperature() const;
	uint8_t getHumidity() const;
	float getBearing() const;
	float getCompassTemperature() const;
	uint8_t getSpeed() const;
	uint16_t getWaterLevel() const;
	bool getWaterPumpActive() const;
	bool getIsDaytime() const;
	double getGpsLatitude() const;
	double getGpsLongitude() const;
	double getGpsAltitude() const;
	double getGpsCourse() const;
	double getGpsDistance() const;
	const char* getGpsDirection() const;
	uint32_t getGpsSatellites() const;
	bool getHornActive() const;
	

	// Setters
	void setTemperature(float value);
	void setHumidity(uint8_t value);
	void setBearing(float value);
	void setCompassTemperature(float value);
	void setSpeed(uint8_t value);
	void setWaterLevel(uint16_t value);
	void setWaterPumpActive(bool value);
	void setDaytime(bool isDaytime);
	void setGpsLocation(double lat, double lon);
	void setGpsAltitude(double alt);
	void setGpsCourse(double course);
	void setGpsSatellites(uint32_t sats);
	void setGpsDirection(const char* dir);
	void setGpsDistance(double distance);
	void setHornActive(bool value);
};

#undef SENSOR_BASE_CLASS