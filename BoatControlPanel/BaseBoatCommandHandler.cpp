#include "BaseBoatCommandHandler.h"

BaseBoatCommandHandler::BaseBoatCommandHandler(
    BroadcastManager* broadcaster,
    NextionControl* nextionControl,
    WarningManager* warningManager
)
    : _broadcaster(broadcaster)
    , _nextionControl(nextionControl)
    , _warningManager(warningManager)
{
}

void BaseBoatCommandHandler::notifyCurrentPage(uint8_t updateType, const void* data)
{
    if (!_nextionControl)
        return;

    BaseDisplayPage* p = _nextionControl->getCurrentPage();
    
    if (!p)
        return;

    p->handleExternalUpdate(updateType, data);
}

void BaseBoatCommandHandler::sendDebugMessage(const String& message, const String& identifier)
{
    if (_broadcaster)
    {
        _broadcaster->sendDebug(message, identifier);
    }
}

void BaseBoatCommandHandler::sendDebugMessage(const char* message, const char* identifier)
{
    if (_broadcaster)
    {
        _broadcaster->sendDebug(message, identifier);
    }
}

void BaseBoatCommandHandler::sendErrorMessage(const String& message, const String& identifier)
{
    if (_broadcaster)
    {
        _broadcaster->sendError(message, identifier);
    }
}

void BaseBoatCommandHandler::sendErrorMessage(const char* message, const char* identifier)
{
    if (_broadcaster)
    {
        _broadcaster->sendError(message, identifier);
    }
}

bool BaseBoatCommandHandler::parseBooleanValue(const String& value) const
{
    return (value == F("1") || 
        value.equalsIgnoreCase(F("on")) || 
        value.equalsIgnoreCase(F("true")));
}

bool BaseBoatCommandHandler::isAllDigits(const String& s) const
{
    if (s.length() == 0) 
        return false;

    for (size_t i = 0; i < s.length(); ++i)
    {
        if (!isDigit(s[i]))
            return false;
    }

    return true;
}
