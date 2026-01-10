#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

#include "SystemDefinitions.h"

#if defined(FUSE_BOX_CONTROLLER)
#include "SmartFuseBoxConstants.h"
#endif
#include "WarningManager.h"
#include "WarningType.h"
#include "BaseSensor.h"

#if defined(MESSAGE_BUS)
#include "MessageBus.h"
#endif

constexpr unsigned long GpsCheckMs = 1000;
constexpr uint32_t GpsBaudRate = 9600;

constexpr unsigned long GpsTimeSyncIntervalMs = 1000UL * 60UL * 5UL; // Sync time every 5 mins

// New sensor command IDs for GPS

/**
 * @brief Sensor handler for GY-GPS6MV2 GPS module monitoring.
 *
 * Reads GPS coordinates, altitude, speed, and satellite count from the GY-GPS6MV2 module,
 * and reports readings to both link and computer serial connections.
 * Uses SoftwareSerial for communication with the GPS module.
 */
class GpsSensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
	Stream* _gpsSerial;
#if defined(MESSAGE_BUS)
	MessageBus* _messageBus;
#endif
	SensorCommandHandler* _sensorCommandHandler;
	WarningManager* _warningManager;
	TinyGPSPlus* _gps;
	double _latitude;
	double _longitude;
	double _altitude;
	double _speedKmh;
	double _courseDeg;
	uint32_t _satellites;
	bool _hasValidFix;
	unsigned long _lastValidData;
	unsigned long _lastTimeSync;

	/**
 * @brief Synchronize DateTimeManager with GPS UTC time.
 * Converts GPS date/time to Unix timestamp and updates DateTimeManager.
 */
	void syncTimeFromGps()
	{
		if (!_gps)
		{
			return;
		}

		// Check if GPS date and time are valid
		if (_gps->date.isValid() && _gps->time.isValid())
		{
			// Extract date/time components
			uint16_t year = _gps->date.year();
			uint8_t month = _gps->date.month();
			uint8_t day = _gps->date.day();
			uint8_t hour = _gps->time.hour();
			uint8_t minute = _gps->time.minute();
			uint8_t second = _gps->time.second();

			// Format as ISO 8601 string: YYYY-MM-DDTHH:MM:SS
			char isoDateTime[20];
			snprintf(isoDateTime, sizeof(isoDateTime), "%04d-%02d-%02dT%02d:%02d:%02d",
				year, month, day, hour, minute, second);

			// Update DateTimeManager with GPS time (UTC)
			if (DateTimeManager::setDateTimeISO(isoDateTime))
			{
				_lastTimeSync = millis();
			}
		}
	}

protected:
	void initialize() override
	{
		_gps = new TinyGPSPlus();

		_lastValidData = millis();
		_lastTimeSync = 0;
	}

	unsigned long update() override
	{
		if (!_gpsSerial || !_gps)
		{
			return GpsCheckMs;
		}

		Serial.print("Receiving data from Gps: ");
		// Process available GPS data
		while (_gpsSerial->available() > 0)
		{
			Serial.print("C:");
			char c = _gpsSerial->read();
			Serial.print(c);
			Serial.print(";");
			_gps->encode(c);
		}
		Serial.println(";");

		// Check if we have a valid location fix
		if (_gps->location.isValid())
		{
			sendDebug("Valid GPS fix", getSensorName());

			_hasValidFix = true;
			_lastValidData = millis();
			
			// Clear any previous GPS failure warnings
			if (_warningManager && _warningManager->isWarningActive(WarningType::SensorFailure))
			{
				_warningManager->clearWarning(WarningType::SensorFailure);
			}

			// Update stored values
			_latitude = _gps->location.lat();
			_longitude = _gps->location.lng();
			_altitude = _gps->altitude.isValid() ? _gps->altitude.meters() : 0.0;
			_speedKmh = _gps->speed.isValid() ? _gps->speed.kmph() : 0.0;
			_courseDeg = _gps->course.isValid() ? _gps->course.deg() : 0.0;
			_satellites = _gps->satellites.isValid() ? _gps->satellites.value() : 0;

			// Sync time from GPS periodically or if never synced
			if (!DateTimeManager::isTimeSet() ||
				(millis() - _lastTimeSync) > GpsTimeSyncIntervalMs)
			{
				syncTimeFromGps();
			}

#if defined(MESSAGE_BUS)
			// Publish to message bus
			if (_messageBus)
			{
				_messageBus->publish<GpsLocationUpdated>(_latitude, _longitude);
				_messageBus->publish<GpsAltitudeUpdated>(_altitude);
				_messageBus->publish<GpsSpeedUpdated>(_speedKmh, _courseDeg);
			}
#endif

			// Send commands to serial
			StringKeyValue params[2];  // Array for lat/long

			// Populate latitude parameter
			strncpy(params[0].key, "lat", sizeof(params[0].key));
			snprintf(params[0].value, sizeof(params[0].value), "%.6f", _latitude);

			// Populate longitude parameter  
			strncpy(params[1].key, "lon", sizeof(params[1].key));
			snprintf(params[1].value, sizeof(params[1].value), "%.6f", _longitude);

			// Send both lat and long as a single command
			sendCommand(SensorGpsLatLong, params, 2);

			// Send altitude
			StringKeyValue altParam;
			strncpy(altParam.key, ValueParamName, sizeof(altParam.key));
			snprintf(altParam.value, sizeof(altParam.value), "%.2f", _altitude);
			sendCommand(SensorGpsAltitude, &altParam, 1);

			// Send speed
			snprintf(altParam.value, sizeof(altParam.value), "%.2f", _speedKmh);
			sendCommand(SensorGpsSpeed, &altParam, 1);

			// Send satellites
			snprintf(altParam.value, sizeof(altParam.value), "%lu", _satellites);
			sendCommand(SensorGpsSatellites, &altParam, 1);
			// Update sensor command handler if available
			if (_sensorCommandHandler)
			{
				_sensorCommandHandler->setGpsLocation(_latitude, _longitude);
				_sensorCommandHandler->setGpsAltitude(_altitude);
				_sensorCommandHandler->setGpsSpeed(_speedKmh);
				_sensorCommandHandler->setGpsSatellites(_satellites);
			}
		}
		else
		{
			sendDebug("No valid GPS fix", getSensorName());
			// Check if GPS data is stale (no valid fix for 30 seconds)
			if (millis() - _lastValidData > 30000)
			{
				if (_warningManager && !_warningManager->isWarningActive(WarningType::GpsFailure))
				{
					_warningManager->raiseWarning(WarningType::GpsFailure);
				}
				_hasValidFix = false;
			}
		}

		return GpsCheckMs;
	}

public:
	GpsSensorHandler(
		Stream* gpsSerial,

#if defined(MESSAGE_BUS)
		MessageBus* messageBus, 
#endif

		BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		WarningManager* warningManager)
		: BroadcastLoggerSupport(broadcastManager), 
		  _gpsSerial(gpsSerial),

#if defined(MESSAGE_BUS)
		_messageBus(messageBus), 
#endif
		  _sensorCommandHandler(sensorCommandHandler),
		  _warningManager(warningManager), 
		  _gps(nullptr),
		  _latitude(0.0), 
		  _longitude(0.0), 
		  _altitude(0.0), 
		  _speedKmh(0.0), 
		  _courseDeg(0.0),
		  _satellites(0),
		  _hasValidFix(false),
		  _lastValidData(0)
	{
	}

	~GpsSensorHandler()
	{
		if (_gps)
		{
			delete _gps;
			_gps = nullptr;
		}
	}

#if defined(FUSE_BOX_CONTROLLER)
	void formatStatusJson(char* buffer, size_t size) override
	{
		char lat[16];
		char lon[16];
		char alt[12];
		char speed[12];
		char course[12];
		
		dtostrf(_latitude, 10, 6, lat);
		dtostrf(_longitude, 10, 6, lon);
		dtostrf(_altitude, 8, 2, alt);
		dtostrf(_speedKmh, 8, 2, speed);
		dtostrf(_courseDeg, 8, 2, course);

		snprintf(buffer, size, 
			"\"gps\":{\"lat\":%s,\"lon\":%s,\"alt\":%s,\"speed\":%s,\"course\":%s,\"sats\":%lu,\"valid\":%s}",
			lat, lon, alt, speed, course, _satellites, _hasValidFix ? "true" : "false");
	}
#endif

	SensorIdList getSensorId() const override
	{
		return SensorIdList::GpsSensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SensorGpsLatLong;
	}

	const char* getSensorName() const override
	{
		return "gps";
	}

	// Public getters for current GPS data
	bool hasValidFix() const { return _hasValidFix; }
	double getLatitude() const { return _latitude; }
	double getLongitude() const { return _longitude; }
	double getAltitude() const { return _altitude; }
	double getSpeed() const { return _speedKmh; }
	double getCourse() const { return _courseDeg; }
	uint32_t getSatellites() const { return _satellites; }
};