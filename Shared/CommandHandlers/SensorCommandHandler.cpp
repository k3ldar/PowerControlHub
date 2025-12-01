#include "SensorCommandHandler.h"

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

bool SensorCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], uint8_t paramCount)
{
    String cmd = command;
    cmd.trim();

    // the first param indicates the value (v=23 or v=NNW)

    if (paramCount == 0)
    {
        sendDebugMessage(F("No parameters in sensor command"), F("SensorCommandHandler"));
        return true;
    }

    String key = params[0].key;
    key.trim();
    String val = params[0].value;
    val.trim();

#if defined(BOAT_CONTROL_PANEL)
    if (cmd == SensorTemperature)
    {
		_lastTemperature = val.toFloat();
        FloatStateUpdate update = { _lastTemperature };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Temperature), &update);
    }
    else if (cmd == SensorHumidity)
    {
		_lastHumidity = static_cast<int16_t>(val.toFloat());
        UInt16Update update = { _lastHumidity };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Humidity), &update);
    }
    else if (cmd == SensorBearing)
    {
		_lastBearing = val.toFloat();
        FloatStateUpdate update = { _lastBearing };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Bearing), &update);
    }
    else if (cmd == SensorDirection)
    {
        CharStateUpdate update = {};
        update.length = min(val.length(), (unsigned int)(CharStateUpdate::MaxLength - 1));
        val.toCharArray(update.value, CharStateUpdate::MaxLength);
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Direction), &update);
    }
    else if (cmd == SensorSpeed)
    {
		_lastSpeed = static_cast<int16_t>(val.toInt());
        UInt16Update update = { _lastSpeed };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Speed), &update);
    }
    else if (cmd == SensorCompassTemp)
    {
		_lastCompassTemp = static_cast<int16_t>(val.toInt());
        FloatStateUpdate update = { val.toFloat() };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CompassTemp), &update);
    }
    else if (cmd == SensorWaterLevel)
    {
        UInt16Update update = { static_cast<uint16_t>(val.toInt()) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterLevel), &update);
    }
    else if (cmd == SensorWaterPumpActive)
    {
        BoolStateUpdate update = { val.toInt() > 0 };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::WaterPumpActive), &update);
    }
    else
    {
        sendDebugMessage(F("Unknown or invalid Sensor command"), F("SensorCommandHandler"));
        return false;
    }
#endif

    sendAckOk(sender, cmd);
    return true;
}

const String* SensorCommandHandler::supportedCommands(size_t& count) const
{
    static const String cmds[] = { SensorTemperature, SensorHumidity, SensorBearing,
        SensorDirection, SensorSpeed, SensorCompassTemp, SensorWaterLevel,
        SensorWaterPumpActive, SensorHornActive };
    count = sizeof(cmds) / sizeof(cmds[0]);
    return cmds;
}