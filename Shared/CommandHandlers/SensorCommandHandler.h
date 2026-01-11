#pragma once

#include <Arduino.h>
#include "ConfigManager.h"
#include "SerialCommandManager.h"
#include "Local.h"

#if defined(FUSE_BOX_CONTROLLER)
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
	double _gpsSpeed;
	uint32_t _gpsSatellites;
public:
#if defined(BOAT_CONTROL_PANEL)
    explicit SensorCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
	explicit SensorCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);
#endif

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

	float getTemperature() const { return _lastTemperature; }
	uint8_t getHumidity() const { return _lastHumidity; }
	float getBearing() const { return _lastBearing; }
	float getCompassTemperature() const { return _lastCompassTemp; }
	uint8_t getSpeed() const { return _lastSpeed; }
	uint16_t getWaterLevel() const { return _lastWaterLevel; }
	bool getWaterPumpActive() const { return _lastWaterPumpActive; }
	bool getIsDaytime() const { return _isDaytime; }
	

	void setTemperature(float value) { _lastTemperature = value; }
	void setHumidity(uint8_t value) { _lastHumidity = value; }
	void setBearing(float value) { _lastBearing = value; }
	void setCompassTemperature(float value) { _lastCompassTemp = value; }
	void setSpeed(uint8_t value) { _lastSpeed = value; }
	void setWaterLevel(uint16_t value) { _lastWaterLevel = value; }
	void setWaterPumpActive(bool value) { _lastWaterPumpActive = value; }
	void setDaytime(bool isDaytime) { _isDaytime = isDaytime; }

	void setGpsLocation(double lat, double lon) { _gpsLatitude = lat; _gpsLongitude = lon; }
	void setGpsAltitude(double alt) { _altitude = alt; }
	void setGpsSpeed(double speed) { _gpsSpeed = speed; }
	void setGpsSatellites(uint32_t sats) { _gpsSatellites = sats; }

	double getGpsLatitude() const { return _gpsLatitude; }
	double getGpsLongitude() const { return _gpsLongitude; }
	double getGpsAltitude() const { return _altitude; }
	double getGpsSpeed() const { return _gpsSpeed; }
	uint32_t getGpsSatellites() const { return _gpsSatellites; }
};

#undef SENSOR_BASE_CLASS