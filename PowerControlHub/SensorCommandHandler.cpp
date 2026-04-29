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
#include "SensorCommandHandler.h"
#include "RemoteSensor.h"
#include "SystemFunctions.h"

SensorCommandHandler::SensorCommandHandler(BroadcastManager* broadcastManager, 
    MessageBus* messageBus,
    WarningManager* warningManager)
    : BaseNextionCommandHandler(broadcastManager, messageBus, warningManager), _remoteSensors(nullptr), _remoteSensorCount(0)
{
}

// Getter implementations
float SensorCommandHandler::getTemperature() const
{
	return _lastTemperature;
}

uint8_t SensorCommandHandler::getHumidity() const
{
	return _lastHumidity;
}

float SensorCommandHandler::getBearing() const
{
	return _lastBearing;
}

float SensorCommandHandler::getCompassTemperature() const
{
	return _lastCompassTemp;
}

uint8_t SensorCommandHandler::getSpeed() const
{
	return _lastSpeed;
}

uint16_t SensorCommandHandler::getWaterLevel() const
{
	return _lastWaterLevel;
}

bool SensorCommandHandler::getWaterPumpActive() const
{
	return _lastWaterPumpActive;
}

bool SensorCommandHandler::getIsDaytime() const
{
	return _isDaytime;
}

double SensorCommandHandler::getGpsLatitude() const
{
	return _gpsLatitude;
}

double SensorCommandHandler::getGpsLongitude() const
{
	return _gpsLongitude;
}

double SensorCommandHandler::getGpsAltitude() const
{
	return _altitude;
}

double SensorCommandHandler::getGpsCourse() const
{
	return _gpsCourse;
}

const char* SensorCommandHandler::getGpsDirection() const
{
	return _gpsDirection;
}

double SensorCommandHandler::getGpsDistance() const
{
    return _gpsDistance;
}

uint32_t SensorCommandHandler::getGpsSatellites() const
{
	return _gpsSatellites;
}

bool SensorCommandHandler::getHornActive() const
{
    return _lastHornActive;
}

// Setter implementations with notifyCurrentPage calls
void SensorCommandHandler::setTemperature(float value)
{
	_lastTemperature = value;

	if (_messageBus)
        _messageBus->publish<TemperatureUpdated>(_lastTemperature);
}

void SensorCommandHandler::setHumidity(uint8_t value)
{
	_lastHumidity = value;

    if (_messageBus)
        _messageBus->publish<HumidityUpdated>(_lastHumidity);
}

void SensorCommandHandler::setBearing(float value)
{
	_lastBearing = value;

    if (_messageBus)
        _messageBus->publish<BearingUpdated>(_lastBearing);
}

void SensorCommandHandler::setCompassTemperature(float value)
{
	_lastCompassTemp = value;

	if (_messageBus)
        _messageBus->publish<CompassTemperatureUpdated>(_lastCompassTemp);
}

void SensorCommandHandler::setSpeed(uint8_t value)
{
	_lastSpeed = value;
	if (_messageBus)
        _messageBus->publish<SpeedUpdated>(static_cast<double>(_lastSpeed));
}

void SensorCommandHandler::setWaterLevel(uint16_t value)
{
	uint16_t lastLevel = _lastWaterLevel;
	_lastWaterLevel = value;

	if (_messageBus)
        _messageBus->publish<WaterLevelUpdated>(_lastWaterLevel, lastLevel);
}

void SensorCommandHandler::setWaterPumpActive(bool value)
{
	_lastWaterPumpActive = value;

    if (_messageBus)
        _messageBus->publish<WaterPumpActiveUpdated>(_lastWaterPumpActive);
}

void SensorCommandHandler::setDaytime(bool isDaytime)
{
	_isDaytime = isDaytime;

    if (_messageBus)
        _messageBus->publish<DaytimeUpdated>(_isDaytime);
}

void SensorCommandHandler::setGpsLocation(double lat, double lon)
{
	_gpsLatitude = lat;
	_gpsLongitude = lon;

	if (_messageBus)
        _messageBus->publish<GpsLocationUpdated>(_gpsLatitude, _gpsLongitude);
}

void SensorCommandHandler::setGpsAltitude(double alt)
{
	_altitude = alt;

    if (_messageBus)
        _messageBus->publish<GpsAltitudeUpdated>(_altitude);
}

void SensorCommandHandler::setGpsCourse(double course)
{
	_gpsCourse = course;

	if (_messageBus)
        _messageBus->publish<BearingUpdated>(static_cast<float>(_gpsCourse));
}

void SensorCommandHandler::setGpsSatellites(uint32_t sats)
{
	_gpsSatellites = sats;

    if (_messageBus)
        _messageBus->publish<GpsSatellitesUpdated>(_gpsSatellites);
}

void SensorCommandHandler::setGpsDirection(const char* dir)
{
	_gpsDirection = dir;

    if (_messageBus)
        _messageBus->publish<GpsDirectionUpdated>(_gpsDirection);
}

void SensorCommandHandler::setGpsDistance(double distance)
{
    _gpsDistance = distance;

    if (_messageBus)
        _messageBus->publish<GpsDistanceUpdated>(_gpsDistance);
}

void SensorCommandHandler::setHornActive(bool value)
{
    _lastHornActive = value;
    
    if (_messageBus)
        _messageBus->publish<HornActiveUpdated>(_lastHornActive);
}

void SensorCommandHandler::setup(RemoteSensor* remoteSensors[], size_t remoteSensorCount)
{
    _remoteSensors = remoteSensors;
	_remoteSensorCount = remoteSensorCount;
}

bool SensorCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    if (_remoteSensors != nullptr && _remoteSensorCount > 0)
    {
        for (size_t i = 0; i < _remoteSensorCount; i++)
        {
            if (_remoteSensors[i] && strcmp(_remoteSensors[i]->getSensorCommandId(), command) == 0)
            {
                _remoteSensors[i]->handleRemoteCommand(params, paramCount);
            }
        }
    }

    // Handle query requests (no parameters) - return current sensor values
    if (paramCount == 0)
    {
        char buffer[64];
        sendDebugMessage(F("Query request - no params"), F("SensorCommandHandler"));

        if (SystemFunctions::commandMatches(command, SensorTemperature))
        {
            // S0 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.1f"), _lastTemperature);
            sender->sendCommand(SensorTemperature, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Temperature"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorHumidity))
        {
            // S1 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastHumidity);
            sender->sendCommand(SensorHumidity, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Humidity"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorBearing))
        {
            // S2 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.1f"), _lastBearing);
            sender->sendCommand(SensorBearing, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Bearing"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorDirection))
        {
            // S3 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%s"), _gpsDirection ? _gpsDirection : "N");
            sender->sendCommand(SensorDirection, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Direction"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorSpeed))
        {
            // S4 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastSpeed);
            sender->sendCommand(SensorSpeed, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Speed"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorCompassTemp))
        {
            // S5 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.1f"), _lastCompassTemp);
            sender->sendCommand(SensorCompassTemp, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Compass Temp"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorWaterLevel))
        {
            // S6 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastWaterLevel);
            sender->sendCommand(SensorWaterLevel, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Water Level"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorWaterPumpActive))
        {
            // S7 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastWaterPumpActive ? 1 : 0);
            sender->sendCommand(SensorWaterPumpActive, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Water Pump"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorHornActive))
        {
            // S8 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _lastHornActive ? 1 : 0);
            sender->sendCommand(SensorHornActive, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Horn Active"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorLightSensor))
        {
            // S9 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%d"), _isDaytime ? 1 : 0);
            sender->sendCommand(SensorLightSensor, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned Light Sensor"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorGpsLatLong))
        {
            // S10 query
            snprintf_P(buffer, sizeof(buffer), PSTR("lat=%.6f&lon=%.6f"), _gpsLatitude, _gpsLongitude);
            sender->sendCommand(SensorGpsLatLong, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned GPS LatLong"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorGpsAltitude))
        {
            // S11 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.2f"), _altitude);
            sender->sendCommand(SensorGpsAltitude, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned GPS Altitude"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorGpsSpeed))
        {
            // S12 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.2f&course=%.2f&dir=%s"),
                (double)_lastSpeed, _gpsCourse, _gpsDirection ? _gpsDirection : "N");
            sender->sendCommand(SensorGpsSpeed, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned GPS Speed"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorGpsSatellites))
        {
            // S13 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%lu"), (unsigned long)_gpsSatellites);
            sender->sendCommand(SensorGpsSatellites, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned GPS Satellites"), F("SensorCommandHandler"));
            return true;
        }
        else if (SystemFunctions::commandMatches(command, SensorGpsDistance))
        {
            // S14 query
            snprintf_P(buffer, sizeof(buffer), PSTR("v=%.2f"), _gpsDistance);
            sender->sendCommand(SensorGpsDistance, buffer);
            sendAckOk(sender, command);
            sendDebugMessage(F("Returned GPS Distance"), F("SensorCommandHandler"));
            return true;
        }

        // Unknown query command
        sendErrorMessage(F("Unknown sensor query command"), "SensorCommandHandler");
        return false;
    }

    if (SystemFunctions::commandMatches(command, SensorTemperature))
    {
        sendDebugMessage(F("Sensor Temperature"), F("SensorCommandHandler"));
        setTemperature(atof(params[0].value));
    }
    else if (SystemFunctions::commandMatches(command, SensorHumidity))
    {
        sendDebugMessage(F("Sensor Humidity"), F("SensorCommandHandler"));
        setHumidity(static_cast<uint8_t>(atof(params[0].value)));
    }
    else if (SystemFunctions::commandMatches(command, SensorBearing))
    {
        sendDebugMessage(F("Sensor Bearing"), F("SensorCommandHandler"));
        setBearing(atof(params[0].value));
    }
    else if (SystemFunctions::commandMatches(command, SensorDirection))
    {
        sendDebugMessage(F("Sensor Direction"), F("SensorCommandHandler"));
        setGpsDirection(params[0].value);
    }
    else if (SystemFunctions::commandMatches(command, SensorSpeed))
    {
        sendDebugMessage(F("Sensor Speed"), F("SensorCommandHandler"));
        setSpeed(static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0)));
    }
    else if (SystemFunctions::commandMatches(command, SensorCompassTemp))
    {
        sendDebugMessage(F("Sensor Compass Temp"), F("SensorCommandHandler"));
        setCompassTemperature(atof(params[0].value));
    }
    else if (SystemFunctions::commandMatches(command, SensorWaterLevel))
    {
        sendDebugMessage(F("Sensor Water Level"), F("SensorCommandHandler"));
        setWaterLevel(static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0)));
    }
    else if (SystemFunctions::commandMatches(command, SensorWaterPumpActive))
    {
        sendDebugMessage(F("Sensor Water Pump Active"), F("SensorCommandHandler"));
        setWaterPumpActive(strtoul(params[0].value, nullptr, 0) > 0);
    }
    else if (SystemFunctions::commandMatches(command, SensorHornActive))
    {
        sendDebugMessage(F("Sensor Horn Active"), F("SensorCommandHandler"));
        setHornActive(strtoul(params[0].value, nullptr, 0) > 0);
    }
    else if (SystemFunctions::commandMatches(command, SensorLightSensor))
    {
        sendDebugMessage(F("Sensor Day Time"), F("SensorCommandHandler"));
        setDaytime(strtoul(params[0].value, nullptr, 0) > 0);
    }
    // Add GPS command handlers
    else if (SystemFunctions::commandMatches(command, SensorGpsLatLong))
    {
        sendDebugMessage(F("GPS LatLong"), F("SensorCommandHandler"));

        // GPS sends lat/lon with named parameters: lat=value&lon=value
        // Need to find which param is lat and which is lon
        if (paramCount >= 2)
        {
            double lat = 0.0;
            double lon = 0.0;
            bool hasLat = false;
            bool hasLon = false;

            for (uint8_t i = 0; i < paramCount; i++)
            {
                if (strcmp(params[i].key, "lat") == 0)
                {
                    lat = atof(params[i].value);
                    hasLat = true;
                }
                else if (strcmp(params[i].key, "lon") == 0)
                {
                    lon = atof(params[i].value);
                    hasLon = true;
                }
            }

            if (hasLat && hasLon)
            {
                setGpsLocation(lat, lon);
            }
        }
    }
    else if (SystemFunctions::commandMatches(command, SensorGpsAltitude))
    {
        sendDebugMessage(F("GPS Altitude"), F("SensorCommandHandler"));
        setGpsAltitude(atof(params[0].value));
    }
    else if (SystemFunctions::commandMatches(command, SensorGpsSpeed))
    {
        sendDebugMessage(F("GPS Speed"), F("SensorCommandHandler"));
        setSpeed(atof(params[0].value));

        // Check for optional course parameter (second parameter)
        if (paramCount >= 2)
        {
            setGpsCourse(atof(params[1].value));
        }

        if (paramCount >= 3)
        {
            setGpsDirection(params[2].value);
        }
    }
    else if (SystemFunctions::commandMatches(command, SensorGpsSatellites))
    {
        sendDebugMessage(F("GPS Satellites"), F("SensorCommandHandler"));
        setGpsSatellites(strtoul(params[0].value, nullptr, 0));
    }
	else if (SystemFunctions::commandMatches(command, SensorGpsDistance))
	{
        sendDebugMessage(F("GPS Distance"), F("SensorCommandHandler"));
        setGpsDistance(atof(params[0].value));
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Sensor command"), F("SensorCommandHandler"));
        return false;
    }

    sendAckOk(sender, command);
    return true;
}

const char* const* SensorCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { SensorTemperature, SensorHumidity, SensorBearing,
        SensorDirection, SensorSpeed, SensorCompassTemp, SensorWaterLevel,
        SensorWaterPumpActive, SensorHornActive, SensorLightSensor, SensorGpsLatLong,
        SensorGpsAltitude, SensorGpsSpeed, SensorGpsSatellites, SensorGpsDistance };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}