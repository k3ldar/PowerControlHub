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
#include <TinyGPS++.h>

#include "SystemDefinitions.h"

#include "PowerControlHubConstants.h"
#include "WarningManager.h"
#include "WarningType.h"
#include "BaseSensor.h"
#include "DateTimeManager.h"
#include "SensorCommandHandler.h"

constexpr unsigned long GpsCheckMs = 10;
constexpr unsigned long StatusUpdateMs = 1000UL;
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
	unsigned long _lastStatusUpdate;
	unsigned long _lastFixTime;

	double _prevLatitude;
	double _prevLongitude;
	double _totalDistanceKm;
	bool _firstFix;

	/**
 * @brief Synchronize DateTimeManager with GPS UTC time.
 * Converts GPS date/time to Unix timestamp and updates DateTimeManager.
 * Supports partial updates: if only date or time is valid, uses existing value from DateTimeManager for the missing component.
 */
	void syncTimeFromGps()
	{
		if (!_gps)
		{
			return;
		}

		bool hasGpsDate = _gps->date.isValid();
		bool hasGpsTime = _gps->time.isValid();

		// Need at least one valid component
		if (!hasGpsDate && !hasGpsTime)
		{
			return;
		}

		// Extract date/time components
		uint16_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;

		// Get date components (from GPS or existing DateTimeManager)
		if (hasGpsDate)
		{
			year = _gps->date.year();
			month = _gps->date.month();
			day = _gps->date.day();
		}
		else
		{
			// Use existing date from DateTimeManager
			year = DateTimeManager::getYear();
			month = DateTimeManager::getMonth();
			day = DateTimeManager::getDay();
		}

		// Get time components (from GPS or existing DateTimeManager)
		if (hasGpsTime)
		{
			hour = _gps->time.hour();
			minute = _gps->time.minute();
			second = _gps->time.second();
		}
		else
		{
			// Use existing time from DateTimeManager
			hour = DateTimeManager::getHour();
			minute = DateTimeManager::getMinute();
			second = DateTimeManager::getSecond();
		}

		// Convert to Unix timestamp using ISO format (setDateTimeISO handles the conversion)
		char isoDateTime[32];
		snprintf_P(isoDateTime, sizeof(isoDateTime), PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
			year, month, day, hour, minute, second);

		// Update DateTimeManager with GPS time (UTC)
		if (DateTimeManager::setDateTimeISO(isoDateTime))
		{
			// Get the Unix timestamp we just set
			unsigned long unixTime = DateTimeManager::getCurrentTime();

			// Send F6 command with Unix timestamp
			StringKeyValue timeParam;
			strncpy(timeParam.key, ValueParamName, sizeof(timeParam.key));
			snprintf_P(timeParam.value, sizeof(timeParam.value), PSTR("%lu"), unixTime);
			sendCommand(SystemSetDateTime, &timeParam, 1);

			_lastTimeSync = millis();
		}
	}

	void processGpsUpdate(unsigned long now)
	{
		if (now - _lastStatusUpdate < StatusUpdateMs)
		{
			return;
		}

		// Send commands to serial
		StringKeyValue params[2];

		strncpy(params[0].key, "lat", sizeof(params[0].key));
		dtostrf(_latitude, 1, 6, params[0].value);

		// Populate longitude parameter  
		strncpy(params[1].key, "lon", sizeof(params[1].key));
		dtostrf(_longitude, 1, 6, params[1].value);

		// Send both lat and long as a single command
		sendCommand(SensorGpsLatLong, params, 2);

		// Send altitude
		StringKeyValue altParam;
		strncpy(altParam.key, ValueParamName, sizeof(altParam.key));
		dtostrf(_altitude, 1, 2, altParam.value);
		sendCommand(SensorGpsAltitude, &altParam, 1);

		// Send speed with course as second parameter
		StringKeyValue speedParams[3];
		strncpy(speedParams[0].key, ValueParamName, sizeof(speedParams[0].key));
		dtostrf(_speedKmh, 1, 2, speedParams[0].value);
		strncpy(speedParams[1].key, "course", sizeof(speedParams[1].key));
		dtostrf(_courseDeg, 1, 2, speedParams[1].value);
		strncpy(speedParams[2].key, "dir", sizeof(speedParams[2].key)); 
		snprintf_P(speedParams[2].value, sizeof(speedParams[2].value), PSTR("%s"), getDirection());
		sendCommand(SensorGpsSpeed, speedParams, 3);

		// Send satellites (integers work fine with snprintf)
		snprintf_P(altParam.value, sizeof(altParam.value), PSTR("%lu"), _satellites);
		sendCommand(SensorGpsSatellites, &altParam, 1);

		// send GPS direction string
		StringKeyValue dirParam;
		strncpy(dirParam.key, ValueParamName, sizeof(dirParam.key));
		strncpy(dirParam.value, getDirection(), sizeof(dirParam.value));
		sendCommand(SensorDirection, &dirParam, 1);
		dtostrf(_courseDeg, 1, 2, dirParam.value);
		sendCommand(SensorBearing, &dirParam, 1);

		// Send total distance
		StringKeyValue distParam;
		strncpy(distParam.key, ValueParamName, sizeof(distParam.key));
		dtostrf(_totalDistanceKm, 1, 2, distParam.value);
		sendCommand(SensorGpsDistance, &distParam, 1);


		// Update sensor command handler if available
		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setGpsLocation(_latitude, _longitude);
			_sensorCommandHandler->setGpsAltitude(_altitude);
			_sensorCommandHandler->setSpeed(_speedKmh);
			_sensorCommandHandler->setGpsCourse(_courseDeg);
			_sensorCommandHandler->setGpsSatellites(_satellites);
			_sensorCommandHandler->setGpsDirection(getDirection());
			_sensorCommandHandler->setGpsDistance(getTotalDistance());
		}

		_lastStatusUpdate = now;
	}

	double calculateDistance(double lat1, double lon1, double lat2, double lon2)
	{
		const double R = 6371.0; // Earth radius in kilometers

		// Convert degrees to radians
		double lat1Rad = lat1 * DEG_TO_RAD;
		double lon1Rad = lon1 * DEG_TO_RAD;
		double lat2Rad = lat2 * DEG_TO_RAD;
		double lon2Rad = lon2 * DEG_TO_RAD;

		// Haversine formula
		double dLat = lat2Rad - lat1Rad;
		double dLon = lon2Rad - lon1Rad;

		double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
			cos(lat1Rad) * cos(lat2Rad) *
			sin(dLon / 2.0) * sin(dLon / 2.0);

		double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

		return R * c;
	}

	double calculateBearing(double lat1, double lon1, double lat2, double lon2)
	{
		double lat1Rad = lat1 * DEG_TO_RAD;
		double lat2Rad = lat2 * DEG_TO_RAD;
		double dLonRad = (lon2 - lon1) * DEG_TO_RAD;

		double y = sin(dLonRad) * cos(lat2Rad);
		double x = cos(lat1Rad) * sin(lat2Rad) -
			sin(lat1Rad) * cos(lat2Rad) * cos(dLonRad);

		double bearing = atan2(y, x) * RAD_TO_DEG;

		if (bearing < 0.0)
		{
			bearing += 360.0;
		}

		return bearing;
	}

protected:
	void initialize() override
	{
		_gps = new TinyGPSPlus();

		_lastValidData = millis();
		_lastTimeSync = 0;
		_lastFixTime = 0;
	}

	uint64_t update() override
	{
		if (!_gpsSerial || !_gps)
		{
			return GpsCheckMs;
		}

		// Process available GPS data
		while (_gpsSerial->available() > 0)
		{
			char c = _gpsSerial->read();
			_gps->encode(c);
		}

		unsigned long now = millis();

		// Check if we have a new location fix (isUpdated resets after read,
		// ensuring we only process each GPS fix once at ~1Hz)
		if (_gps->location.isUpdated())
		{
			_hasValidFix = true;
			_lastValidData = now;
			
			if (_warningManager && _warningManager->isWarningActive(WarningType::GpsFailure))
			{
				_warningManager->clearWarning(WarningType::GpsFailure);
			}

			double newLat = _gps->location.lat();
			double newLon = _gps->location.lng();

			// Calculate speed and course from distance between readings
			if (!_firstFix)
			{
				double segmentDistance = calculateDistance(_prevLatitude, _prevLongitude, newLat, newLon);

				// Only update if movement is reasonable (filter out GPS noise)
				if (segmentDistance > 0.0015)
				{
					_totalDistanceKm += segmentDistance;

					// Calculate speed from distance and elapsed time
					unsigned long timeDelta = now - _lastFixTime;
					if (timeDelta > 0)
					{
						double hours = timeDelta / 3600000.0;
						_speedKmh = segmentDistance / hours;
					}

					// Calculate course from bearing between positions
					_courseDeg = calculateBearing(_prevLatitude, _prevLongitude, newLat, newLon);
				}
				else
				{
					// Not enough movement to determine speed/course
					_speedKmh = 0.0;
				}
			}
			else
			{
				_firstFix = false;
				_speedKmh = 0.0;
			}

			_lastFixTime = now;

			// Store current position for next calculation
			_prevLatitude = newLat;
			_prevLongitude = newLon;

			// Update other stored values
			_latitude = newLat;
			_longitude = newLon;
			_altitude = _gps->altitude.isValid() ? _gps->altitude.meters() : 0.0;
			_satellites = _gps->satellites.isValid() ? _gps->satellites.value() : 0;

			if (_courseDeg >= 360.0)
			{
				_courseDeg = fmod(_courseDeg, 360.0);
			}
			else if (_courseDeg < 0.0)
			{
				_courseDeg += 360.0;
			}

			// Sync time from GPS periodically or if never synced
			if (!DateTimeManager::isTimeSet() ||
				(now - _lastTimeSync) > GpsTimeSyncIntervalMs)
			{
				syncTimeFromGps();
			}

			processGpsUpdate(now);
		}
		else
		{
			// Check if GPS data is stale (no valid fix for 30 seconds)
			if (now - _lastValidData > 30000)
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

		BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler, WarningManager* warningManager, const char* name = "Gps")
		: BaseSensor(name), BroadcastLoggerSupport(broadcastManager), 
		  _gpsSerial(gpsSerial),
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
		_lastValidData(0),
		_lastStatusUpdate(0),
		_lastFixTime(0),
		_prevLatitude(0.0),
		_prevLongitude(0.0),
		_totalDistanceKm(0.0),
		_firstFix(true)
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

	void formatStatusJson(char* buffer, size_t size) override
	{
		char lat[16];
		char lon[16];
		char alt[12];
		char speed[12];
		char course[12];
		
		dtostrf(_latitude, 1, 6, lat);
		dtostrf(_longitude, 1, 6, lon);
		dtostrf(_altitude, 1, 2, alt);
		dtostrf(_speedKmh, 1, 2, speed);
		dtostrf(_courseDeg, 1, 2, course);

		snprintf_P(buffer, size, 
			PSTR("\"gps\":{\"lat\":%s,\"lon\":%s,\"alt\":%s,\"speed\":%s,\"course\":%s,\"sats\":%lu,\"valid\":%s}"),
			lat, lon, alt, speed, course, _satellites, _hasValidFix ? "true" : "false");
	}

	SensorIdList getSensorIdType() const override
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
	const char* getDirection() const { return compassDirections[static_cast<int>((_courseDeg + 11.25) / 22.5) % 16]; }
	double getTotalDistance() const { return _totalDistanceKm; }
    
	void resetTotalDistance()
	{ 
		_totalDistanceKm = 0.0; 
		_firstFix = true;
	}

#if defined(MQTT_SUPPORT)
	uint8_t getMqttChannelCount() const override
	{
		return 6;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		switch (channelIndex)
		{
			case 0: return { "GPS Latitude",   "gps_latitude",   "gps_latitude",   nullptr, "°",   false };
			case 1: return { "GPS Longitude",  "gps_longitude",  "gps_longitude",  nullptr, "°",   false };
			case 2: return { "GPS Altitude",   "gps_altitude",   "gps_altitude",   nullptr, "m",   false };
			case 3: return { "GPS Speed",      "gps_speed",      "gps_speed",      nullptr, "km/h",false };
			case 4: return { "GPS Satellites", "gps_satellites", "gps_satellites", nullptr, nullptr,false };
			case 5: return { "GPS Fix",        "gps_fix",        "gps_fix",        nullptr, nullptr,true };
			default: return { nullptr, nullptr, nullptr, nullptr, nullptr, false };
		}
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		switch (channelIndex)
		{
			case 0: dtostrf(_latitude,  1, 6, buffer); break;
			case 1: dtostrf(_longitude, 1, 6, buffer); break;
			case 2: dtostrf(_altitude,  1, 2, buffer); break;
			case 3: dtostrf(_speedKmh,  1, 2, buffer); break;
			case 4: snprintf(buffer, size, "%lu", _satellites); break;
			case 5: snprintf(buffer, size, "%s", _hasValidFix ? "true" : "false"); break;
			default: if (size > 0) buffer[0] = '\0'; break;
		}
	}
#endif
};