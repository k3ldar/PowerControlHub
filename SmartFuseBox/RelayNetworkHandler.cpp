#include "RelayNetworkHandler.h"

RelayNetworkHandler::RelayNetworkHandler(RelayController* relayController)
    : _relayController(relayController)
{
}

CommandResult RelayNetworkHandler::handleRequest(const String& method, 
    const String& body, char* responseBuffer, size_t bufferSize)
{
    if (!_relayController)
    {
        formatJsonResponse(responseBuffer, bufferSize, false, "Controller not initialized");
        return CommandResult::error(RelayControllerNotInitialised);
    }
    
    // GET /api/relay - Get all relay states
    if (method == "GET" && body.length() == 0)
    {
        formatRelayStatesJson(responseBuffer, bufferSize);
        return CommandResult::ok();
    }
    
    // POST /api/relay/all?state=0|1 - Turn all on/off
    if (method == "POST" && body.startsWith("state="))
    {
        bool state = body.substring(6).toInt() > 0;
        
        if (state)
            _relayController->turnAllRelaysOn();
        else
            _relayController->turnAllRelaysOff();
            
        formatRelayStatesJson(responseBuffer, bufferSize);
        return CommandResult::ok();
    }
    
    // PUT /api/relay/:id body: {"state": 0|1}
    if (method == "PUT" && body.length() > 0)
    {
        // Parse body: "id=3&state=1"
        int idStart = body.indexOf("id=");
        int stateStart = body.indexOf("state=");
        
        if (idStart >= 0 && stateStart >= 0)
        {
            uint8_t relayId = body.substring(idStart + 3, body.indexOf('&', idStart)).toInt();
            bool state = body.substring(stateStart + 6).toInt() > 0;
            
            CommandResult result = _relayController->setRelayState(relayId, state);
            
            if (result.success)
            {
                formatRelayStatesJson(responseBuffer, bufferSize);
                return CommandResult::ok();
            }
            else
            {
                RelayResult err = static_cast<RelayResult>(result.status);
                const char* msg = (err == RelayResult::InvalidIndex) ? "Invalid index" : "Reserved relay";
                formatJsonResponse(responseBuffer, bufferSize, false, msg);
                return result;
            }
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