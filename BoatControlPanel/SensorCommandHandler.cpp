#include "SensorCommandHandler.h"

SensorCommandHandler::SensorCommandHandler(BroadcastManager* broadcastManager, NextionControl* nextionControl, WarningManager* warningManager)
    : BaseBoatCommandHandler(broadcastManager, nextionControl, warningManager)
{
}

bool SensorCommandHandler::handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount)
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

    if (cmd == SensorTemperature)
    {
        FloatStateUpdate update = { val.toFloat() };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Temperature), &update);
    }
    else if (cmd == SensorHumidity)
    {
        IntStateUpdate update = { static_cast<int16_t>(val.toInt()) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Humidity), &update);
    }
    else if (cmd == SensorBearing)
    {
        FloatStateUpdate update = { val.toFloat() };
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
        IntStateUpdate update = { static_cast<int16_t>(val.toInt()) };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::Speed), &update);
    }
    else if (cmd == SensorCompassTemp)
    {
        FloatStateUpdate update = { val.toFloat() };
        notifyCurrentPage(static_cast<uint8_t>(PageUpdateType::CompassTemp), &update);
    }
    else if (cmd == SensorWaterLevel)
    {
        IntStateUpdate update = { static_cast<int16_t>(val.toInt()) };
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