#pragma once

#include <Arduino.h>
#include "ConfigManager.h"
#include "SerialCommandManager.h"
#include "Local.h"

#ifdef FUSE_BOX_CONTROLLER
#include "SharedBaseCommandHandler.h"
#endif

#ifdef BOAT_CONTROL_PANEL
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
public:
#if defined(BOAT_CONTROL_PANEL)
    explicit SensorCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager);
#elif defined(FUSE_BOX_CONTROLLER)
	explicit SensorCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager);
#endif

    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
    const String* supportedCommands(size_t& count) const override;

	float getTemperature() const { return _lastTemperature; }
	uint8_t getHumidity() const { return _lastHumidity; }
	float getBearing() const { return _lastBearing; }
	uint8_t getCompassTemperature() const { return _lastCompassTemp; }
	uint8_t getSpeed() const { return _lastSpeed; }
	uint16_t getWaterLevel() const { return _lastWaterLevel; }
	bool getWaterPumpActive() const { return _lastWaterPumpActive; }
	

	void setTemperature(float value) { _lastTemperature = value; }
	void setHumidity(uint8_t value) { _lastHumidity = value; }
	void setBearing(float value) { _lastBearing = value; }
	void setCompassTemperature(uint8_t value) { _lastCompassTemp = value; }
	void setSpeed(uint8_t value) { _lastSpeed = value; }
	void setWaterLevel(uint16_t value) { _lastWaterLevel = value; }
	void setWaterPumpActive(bool value) { _lastWaterPumpActive = value; }
private:
	float _lastTemperature = NAN;
	uint8_t _lastHumidity = NAN;
	float _lastBearing = NAN;
	uint8_t _lastCompassTemp = 0;
	uint8_t _lastSpeed = 0;
	uint16_t _lastWaterLevel = 0;
	bool _lastWaterPumpActive = -1;
};

#undef SENSOR_BASE_CLASS