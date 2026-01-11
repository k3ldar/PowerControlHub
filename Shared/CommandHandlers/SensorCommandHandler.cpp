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

bool SensorCommandHandler::handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount)
{
#if defined(FUSE_BOX_CONTROLLER)
    (void)command;
#endif
    // the first param indicates the value (v=23 or v=NNW)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in sensor command"), F("SensorCommandHandler"));
        return true;
    }

#if defined(BOAT_CONTROL_PANEL)
    if (strcmp(command, SensorTemperature) == 0)
    {
        sendDebugMessage(F("Sensor Temperature"), F("SensorCommandHandler"));
        _lastTemperature = atof(params[0].value);
        FloatStateUpdate update = { _lastTemperature };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Temperature), &update);
    }
    else if (strcmp(command, SensorHumidity) == 0)
    {
        sendDebugMessage(F("Sensor Humidity"), F("SensorCommandHandler"));
        _lastHumidity = static_cast<int16_t>(atof(params[0].value));
        UInt16Update update = { _lastHumidity };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Humidity), &update);
    }
    else if (strcmp(command, SensorBearing) == 0)
    {
        sendDebugMessage(F("Sensor Bearing"), F("SensorCommandHandler"));
        _lastBearing = atof(params[0].value);
        FloatStateUpdate update = { _lastBearing };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Bearing), &update);
    }
    else if (strcmp(command, SensorDirection) == 0)
    {
        sendDebugMessage(F("Sensor Direction"), F("SensorCommandHandler"));
        CharStateUpdate update = {};
        update.length = min(SystemFunctions::calculateLength(params[0].value), static_cast<unsigned int>((CharStateUpdate::MaxLength - 1)));
		snprintf(update.value, CharStateUpdate::MaxLength, "%s", params[0].value);
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Direction), &update);
    }
    else if (strcmp(command, SensorSpeed) == 0)
    {
        sendDebugMessage(F("Sensor Speed"), F("SensorCommandHandler"));
        _lastSpeed = static_cast<int16_t>(strtoul(params[0].value, nullptr, 0));
        UInt16Update update = { _lastSpeed };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Speed), &update);
    }
    else if (strcmp(command, SensorCompassTemp) == 0)
    {
        sendDebugMessage(F("Sensor Compass Temp"), F("SensorCommandHandler"));
        _lastCompassTemp = static_cast<int16_t>(strtoul(params[0].value, nullptr, 0));
        FloatStateUpdate update = { atof(params[0].value) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CompassTemp), &update);
    }
    else if (strcmp(command, SensorWaterLevel) == 0)
    {
        sendDebugMessage(F("Sensor Water Level"), F("SensorCommandHandler"));
        UInt16Update update = { static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0)) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterLevel), &update);
    }
    else if (strcmp(command, SensorWaterPumpActive) == 0)
    {
        sendDebugMessage(F("Sensor Water Pump Active"), F("SensorCommandHandler"));
        BoolStateUpdate update = { strtoul(params[0].value, nullptr, 0) > 0 };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterPumpActive), &update);
    }
    else if (strcmp(command, SensorLightSensor) == 0)
    {
        sendDebugMessage(F("Sensor Day Time"), F("SensorCommandHandler"));
        BoolStateUpdate update = { strtoul(params[0].value, nullptr, 0) > 0 };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Daytime), &update);
    }
    // Add GPS command handlers
    else if (strcmp(command, SensorGpsLatLong) == 0)
    {
        sendDebugMessage(F("GPS LatLong"), F("SensorCommandHandler"));
        
        // GPS sends lat/lon with named parameters: lat=value&lon=value
        // Need to find which param is lat and which is lon
        if (paramCount >= 2)
        {
            for (uint8_t i = 0; i < paramCount; i++)
            {
                if (strcmp(params[i].key, "lat") == 0)
                {
                    _gpsLatitude = atof(params[i].value);
                    FloatStateUpdate updateLat = { static_cast<float>(_gpsLatitude) };
                    notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsLatitude), &updateLat);
                }
                else if (strcmp(params[i].key, "lon") == 0)
                {
                    _gpsLongitude = atof(params[i].value);
                    FloatStateUpdate updateLong = { static_cast<float>(_gpsLongitude) };
                    notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsLongitude), &updateLong);
                }
            }
        }
    }
    else if (strcmp(command, SensorGpsAltitude) == 0)
    {
        sendDebugMessage(F("GPS Altitude"), F("SensorCommandHandler"));
        _altitude = atof(params[0].value);
        FloatStateUpdate update = { static_cast<float>(_altitude) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsAltitude), &update);
    }
    else if (strcmp(command, SensorGpsSpeed) == 0)
    {
        sendDebugMessage(F("GPS Speed"), F("SensorCommandHandler"));
        _gpsSpeed = atof(params[0].value);
        FloatStateUpdate update = { static_cast<float>(_gpsSpeed) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsSpeed), &update);
    }
    else if (strcmp(command, SensorGpsSatellites) == 0)
    {
        sendDebugMessage(F("GPS Satellites"), F("SensorCommandHandler"));
        _gpsSatellites = strtoul(params[0].value, nullptr, 0);
        UInt16Update update = { static_cast<uint16_t>(_gpsSatellites) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::GpsSatellites), &update);
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Sensor command"), F("SensorCommandHandler"));
        return false;
    }
#endif

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