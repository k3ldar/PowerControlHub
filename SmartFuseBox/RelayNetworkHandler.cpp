#include "RelayNetworkHandler.h"

RelayNetworkHandler::RelayNetworkHandler(RelayController* relayController)
	: _relayController(relayController)
{
}

CommandResult RelayNetworkHandler::handleRequest(const String& method,
    const String& command, StringKeyValue* params, uint8_t paramCount,
    char* responseBuffer, size_t bufferSize)
{
    if (!_relayController)
    {
        formatJsonResponse(responseBuffer, bufferSize, false, "Controller not initialized");
        return CommandResult::error(RelayControllerNotInitialised);
    }

    String cmd = command;
    cmd.trim();

    if (cmd == RelayTurnAllOff)
    {
        _relayController->turnAllRelaysOff();
        formatRelayStatesJson(responseBuffer, bufferSize);
		return CommandResult::ok();
    }
    else if (cmd == RelayTurnAllOn)
    {
        _relayController->turnAllRelaysOn();
		formatRelayStatesJson(responseBuffer, bufferSize);
		return CommandResult::ok();
    }
    else if (cmd == RelayRetrieveStates)
    {
		formatRelayStatesJson(responseBuffer, bufferSize);
        return CommandResult::ok();
    }
    else if (cmd == RelaySetState)
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = params[0].key.toInt();
            uint8_t state = params[0].value.toInt();

            if (relayIndex >= _relayController->getRelayCount())
            {
                return CommandResult::error(InvalidCommandParameters);
            }

            CommandResult result = _relayController->setRelayState(relayIndex, state > 0);
            RelayResult status = static_cast<RelayResult>(result.status);

            if (status == RelayResult::InvalidIndex)
            {
                return CommandResult::error(InvalidCommandParameters);
            }
            else if (status == RelayResult::Reserved)
            {
                return CommandResult::error(InvalidCommandParameters);
            }

            formatRelayStatesJson(responseBuffer, bufferSize);
            return CommandResult::ok();
        }
        else
        {
			return CommandResult::error(InvalidCommandParameters);
        }
    }
    else if (cmd == RelayStatusGet)
    {
        if (paramCount == 1)
        {
            uint8_t relayIndex = params[0].key.toInt();

            if (relayIndex >= _relayController->getRelayCount())
            {
                return CommandResult::error(InvalidCommandParameters);
            }

            CommandResult result = _relayController->getRelayStatus(relayIndex);
            uint8_t status = result.status;
            StringKeyValue param = { String(relayIndex), String(status) };

            formatRelayStatesJson(responseBuffer, bufferSize);
            return CommandResult::ok();
        }
        else
        {
            return CommandResult::error(InvalidCommandParameters);
        }
    }
    
    formatJsonResponse(responseBuffer, bufferSize, false, "Invalid request");
    return CommandResult{ false, 0 };
}

void RelayNetworkHandler::formatRelayStatesJson(char* buffer, size_t size)
{
    // Simple JSON formatting without library (to save memory)
    int written = snprintf(buffer, size, "{\"relays\":[");
    
    for (uint8_t i = 0; i < _relayController->getRelayCount(); i++)
    {
        CommandResult result = _relayController->getRelayStatus(i);
        written += snprintf(buffer + written, size - written, 
                          "%d%s", result.status, 
                          (i < _relayController->getRelayCount() - 1) ? "," : "");
    }
    
    snprintf(buffer + written, size - written, "]}");
}

void RelayNetworkHandler::formatJsonResponse(char* buffer, size_t size, bool success, const char* message)
{
    if (message)
    {
        snprintf(buffer, size, "{\"success\":%s,\"message\":\"%s\"}", 
                success ? "true" : "false", message);
    }
    else
    {
        snprintf(buffer, size, "{\"success\":%s}", success ? "true" : "false");
    }
}