#include "BroadcastManager.h"

constexpr unsigned long UpdateIntervalMs = 5000;

BroadcastManager::BroadcastManager(SerialCommandManager* computerSerial, SerialCommandManager* linkSerial)
	: _computerSerial(computerSerial), _linkSerial(linkSerial), _nextUpdateTime(0)
{
}

void BroadcastManager::update(unsigned long now)
{
    // Perform periodic updates every 5 seconds: send configuration values to serial interfaces
	if (now > _nextUpdateTime)
    {
        _nextUpdateTime = now + UpdateIntervalMs;

		Config* config = ConfigManager::getConfigPtr();
        
        if (!config)
			return;

        sendCommand(ConfigSoundRelayId, "v=" + String(config->hornRelayIndex), true);
        sendCommand(ConfigBoatType, "v=" + String(static_cast<int>(config->vesselType)), true);
    }
}

void BroadcastManager::sendCommand(const char* command, const char* params, bool linkOnly)
{
    if (_computerSerial && !linkOnly)
    {
        _computerSerial->sendCommand(command, params);
    }

    if (_linkSerial)
    {
        _linkSerial->sendCommand(command, params);
    }
}

void BroadcastManager::sendCommand(const String& command, const String& params, bool linkOnly)
{
    sendCommand(command.c_str(), params.c_str(), linkOnly);
}

void BroadcastManager::sendDebug(const char* message, const char* source)
{
    if (_computerSerial)
    {
        _computerSerial->sendDebug(message, source);
    }
}

void BroadcastManager::sendDebug(const String& message, const String& source)
{
    sendDebug(message.c_str(), source.c_str());
}

void BroadcastManager::sendError(const char* message, const char* source)
{
    if (_computerSerial)
    {
        _computerSerial->sendError(message, source);
    }
}

void BroadcastManager::sendError(const String& message, const String& source)
{
    sendError(message.c_str(), source.c_str());
}