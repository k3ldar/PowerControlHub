#include "SensorCommandHandler.h"
#include "SystemFunctions.h"

#if defined(BOAT_CONTROL_PANEL)
SensorCommandHandler::SensorCommandHandler(BroadcastManager* broadcastManager, 
	NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
#elif defined(FUSE_BOX_CONTROLLER)
SensorCommandHandler::SensorCommandHandler(BroadcastManager* broadcastManager, WarningManager* warningManager)
    : SharedBaseCommandHandler(broadcastManager, warningManager)
#endif
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

double SensorCommandHandler::getGpsSpeed() const
{
	return _gpsSpeed;
}

double SensorCommandHandler::getGpsCourse() const
{
	return _gpsCourse;
}

const char* SensorCommandHandler::getGpsDirection() const
{
	return _gpsDirection;
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
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { _lastTemperature };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Temperature), &update);
#endif
}

void SensorCommandHandler::setHumidity(uint8_t value)
{
	_lastHumidity = value;
#if defined(BOAT_CONTROL_PANEL)
	UInt16Update update = { _lastHumidity };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Humidity), &update);
#endif
}

void SensorCommandHandler::setBearing(float value)
{
	_lastBearing = value;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { _lastBearing };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Bearing), &update);
#endif
}

void SensorCommandHandler::setCompassTemperature(float value)
{
	_lastCompassTemp = value;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { _lastCompassTemp };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CompassTemp), &update);
#endif
}

void SensorCommandHandler::setSpeed(uint8_t value)
{
	_lastSpeed = value;
#if defined(BOAT_CONTROL_PANEL)
	UInt16Update update = { _lastSpeed };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Speed), &update);
#endif
}

void SensorCommandHandler::setWaterLevel(uint16_t value)
{
	_lastWaterLevel = value;
#if defined(BOAT_CONTROL_PANEL)
	UInt16Update update = { _lastWaterLevel };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterLevel), &update);
#endif
}

void SensorCommandHandler::setWaterPumpActive(bool value)
{
	_lastWaterPumpActive = value;
#if defined(BOAT_CONTROL_PANEL)
	BoolStateUpdate update = { _lastWaterPumpActive };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterPumpActive), &update);
#endif
}

void SensorCommandHandler::setDaytime(bool isDaytime)
{
	_isDaytime = isDaytime;
#if defined(BOAT_CONTROL_PANEL)
	BoolStateUpdate update = { _isDaytime };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Daytime), &update);
#endif
}

void SensorCommandHandler::setGpsLocation(double lat, double lon)
{
	_gpsLatitude = lat;
	_gpsLongitude = lon;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate updateLat = { static_cast<float>(_gpsLatitude) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsLatitude), &updateLat);
	
	FloatStateUpdate updateLon = { static_cast<float>(_gpsLongitude) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsLongitude), &updateLon);
#endif
}

void SensorCommandHandler::setGpsAltitude(double alt)
{
	_altitude = alt;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { static_cast<float>(_altitude) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsAltitude), &update);
#endif
}

void SensorCommandHandler::setGpsSpeed(double speed)
{
	_gpsSpeed = speed;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { static_cast<float>(_gpsSpeed) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsSpeed), &update);
#endif
}

void SensorCommandHandler::setGpsCourse(double course)
{
	_gpsCourse = course;
#if defined(BOAT_CONTROL_PANEL)
	FloatStateUpdate update = { static_cast<float>(_gpsCourse) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsCourse), &update);
#endif
}

void SensorCommandHandler::setGpsSatellites(uint32_t sats)
{
	_gpsSatellites = sats;
#if defined(BOAT_CONTROL_PANEL)
	UInt16Update update = { static_cast<uint16_t>(_gpsSatellites) };
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsSatellites), &update);
#endif
}

void SensorCommandHandler::setGpsDirection(const char* dir)
{
	_gpsDirection = dir;
#if defined(BOAT_CONTROL_PANEL)
	CharStateUpdate update = {};
	update.length = min(SystemFunctions::calculateLength(dir), static_cast<unsigned int>((CharStateUpdate::MaxLength - 1)));
	snprintf(update.value, CharStateUpdate::MaxLength, "%s", dir);
	notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Direction), &update);
#endif
}

void SensorCommandHandler::setHornActive(bool value)
{
    _lastHornActive = value;
#if defined(BOAT_CONTROL_PANEL)
    BoolStateUpdate update = { _lastHornActive };
    notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::HornActive), &update);
#endif
}

bool SensorCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
    // the first param indicates the value (v=23 or v=NNW)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in sensor command"), F("SensorCommandHandler"));
        return true;
    }

    if (strcmp(command, SensorTemperature) == 0)
    {
        sendDebugMessage(F("Sensor Temperature"), F("SensorCommandHandler"));
        setTemperature(atof(params[0].value));
    }
    else if (strcmp(command, SensorHumidity) == 0)
    {
        sendDebugMessage(F("Sensor Humidity"), F("SensorCommandHandler"));
        setHumidity(static_cast<uint8_t>(atof(params[0].value)));
    }
    else if (strcmp(command, SensorBearing) == 0)
    {
        sendDebugMessage(F("Sensor Bearing"), F("SensorCommandHandler"));
        setBearing(atof(params[0].value));
    }
    else if (strcmp(command, SensorDirection) == 0)
    {
        sendDebugMessage(F("Sensor Direction"), F("SensorCommandHandler"));
        setGpsDirection(params[0].value);
    }
    else if (strcmp(command, SensorSpeed) == 0)
    {
        sendDebugMessage(F("Sensor Speed"), F("SensorCommandHandler"));
        setSpeed(static_cast<uint8_t>(strtoul(params[0].value, nullptr, 0)));
    }
    else if (strcmp(command, SensorCompassTemp) == 0)
    {
        sendDebugMessage(F("Sensor Compass Temp"), F("SensorCommandHandler"));
        setCompassTemperature(atof(params[0].value));
    }
    else if (strcmp(command, SensorWaterLevel) == 0)
    {
        sendDebugMessage(F("Sensor Water Level"), F("SensorCommandHandler"));
        setWaterLevel(static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0)));
    }
    else if (strcmp(command, SensorWaterPumpActive) == 0)
    {
        sendDebugMessage(F("Sensor Water Pump Active"), F("SensorCommandHandler"));
        setWaterPumpActive(strtoul(params[0].value, nullptr, 0) > 0);
    }
    else if (strcmp(command, SensorHornActive) == 0)
    {
        sendDebugMessage(F("Sensor Horn Active"), F("SensorCommandHandler"));
        setHornActive(strtoul(params[0].value, nullptr, 0) > 0);
    }
    else if (strcmp(command, SensorLightSensor) == 0)
    {
        sendDebugMessage(F("Sensor Day Time"), F("SensorCommandHandler"));
        setDaytime(strtoul(params[0].value, nullptr, 0) > 0);
    }
    // Add GPS command handlers
    else if (strcmp(command, SensorGpsLatLong) == 0)
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
    else if (strcmp(command, SensorGpsAltitude) == 0)
    {
        sendDebugMessage(F("GPS Altitude"), F("SensorCommandHandler"));
        setGpsAltitude(atof(params[0].value));
    }
    else if (strcmp(command, SensorGpsSpeed) == 0)
    {
        sendDebugMessage(F("GPS Speed"), F("SensorCommandHandler"));
        setGpsSpeed(atof(params[0].value));
        
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
    else if (strcmp(command, SensorGpsSatellites) == 0)
    {
        sendDebugMessage(F("GPS Satellites"), F("SensorCommandHandler"));
        setGpsSatellites(strtoul(params[0].value, nullptr, 0));
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Sensor command"), F("SensorCommandHandler"));
        return false;
    }

    sendAckOk(sender, params[0].key);
    return true;
}

const char* const* SensorCommandHandler::supportedCommands(size_t& count) const
{
    static const char* cmds[] = { SensorTemperature, SensorHumidity, SensorBearing,
        SensorDirection, SensorSpeed, SensorCompassTemp, SensorWaterLevel,
        SensorWaterPumpActive, SensorHornActive, SensorLightSensor, SensorGpsLatLong,
        SensorGpsAltitude, SensorGpsSpeed, SensorGpsSatellites };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}