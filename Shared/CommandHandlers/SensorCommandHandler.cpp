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
    // the first param indicates the value (v=23 or v=NNW)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in sensor command"), F("SensorCommandHandler"));
        return true;
    }

#if defined(BOAT_CONTROL_PANEL)
    if (command == SensorTemperature)
    {
		_lastTemperature = atof(params[0].value);
        FloatStateUpdate update = { _lastTemperature };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Temperature), &update);
    }
    else if (command == SensorHumidity)
    {
		_lastHumidity = static_cast<int16_t>(atof(params[0].value));
        UInt16Update update = { _lastHumidity };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Humidity), &update);
    }
    else if (command == SensorBearing)
    {
		_lastBearing = atof(params[0].value);
        FloatStateUpdate update = { _lastBearing };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Bearing), &update);
    }
    else if (command == SensorDirection)
    {
        CharStateUpdate update = {};
        update.length = min(SystemFunctions::calculateLength(params[0].value), static_cast<unsigned int>((CharStateUpdate::MaxLength - 1)));
		snprintf(update.value, CharStateUpdate::MaxLength, "%s", params[0].value);
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Direction), &update);
    }
    else if (command == SensorSpeed)
    {
		_lastSpeed = static_cast<int16_t>(strtoul(params[0].value, nullptr, 0));
        UInt16Update update = { _lastSpeed };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Speed), &update);
    }
    else if (command == SensorCompassTemp)
    {
		_lastCompassTemp = static_cast<int16_t>(strtoul(params[0].value, nullptr, 0));
        FloatStateUpdate update = { atof(params[0].value) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CompassTemp), &update);
    }
    else if (command == SensorWaterLevel)
    {
        UInt16Update update = { static_cast<uint16_t>(strtoul(params[0].value, nullptr, 0)) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterLevel), &update);
    }
    else if (command == SensorWaterPumpActive)
    {
        BoolStateUpdate update = { strtoul(params[0].value, nullptr, 0) > 0 };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterPumpActive), &update);
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
        SensorWaterPumpActive, SensorHornActive };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}