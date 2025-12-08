#pragma once
#include <SerialCommandManager.h>
#include <Arduino.h>
#include "SystemDefinitions.h"
#include "JsonVisitor.h"



/**
 * @brief Interface for handlers that support network protocols (WiFi, future Bluetooth mesh, etc.)
 * 
 * Handlers implement this to expose REST-style endpoints for HTTP/WebSocket access.
 */
class INetworkCommandHandler : public JsonVisitor
{
public:
    /**
     * @brief Get the HTTP route pattern (e.g., "/api/relay")
     */
    virtual const char* getRoute() const = 0;
    
    /**
     * @brief Handle HTTP request
     * 
     * @param method HTTP method (GET, POST, PUT, DELETE)
	 * @param command Command extracted from URL path
	 * @param params array of HTTP parameters
	 * @param paramCount number of parameters in params array
     * @param responseBuffer Buffer to write response (JSON)
     * @param bufferSize Size of response buffer
     * @return CommandResult with success/status
     */
    virtual CommandResult handleRequest(const String& method, 
		const String& command,
        StringKeyValue* params,
		uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) = 0;
    
    virtual ~INetworkCommandHandler() = default;

    void formatJsonResponse(char* buffer, size_t size, bool success, const char* message = nullptr)
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

};