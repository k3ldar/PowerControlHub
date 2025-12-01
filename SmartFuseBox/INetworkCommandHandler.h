#pragma once
#include <Arduino.h>
#include "SharedConstants.h"

/**
 * @brief Interface for handlers that support network protocols (WiFi, future Bluetooth mesh, etc.)
 * 
 * Handlers implement this to expose REST-style endpoints for HTTP/WebSocket access.
 */
class INetworkCommandHandler {
public:
    /**
     * @brief Get the HTTP route pattern (e.g., "/api/relay")
     */
    virtual const char* getRoute() const = 0;
    
    /**
     * @brief Handle HTTP request
     * 
     * @param method HTTP method (GET, POST, PUT, DELETE)
     * @param body Request body (JSON or form data)
     * @param responseBuffer Buffer to write response (JSON)
     * @param bufferSize Size of response buffer
     * @return CommandResult with success/status
     */
    virtual CommandResult handleRequest(const String& method, 
                                       const String& body,
                                       char* responseBuffer,
                                       size_t bufferSize) = 0;
    
    virtual ~INetworkCommandHandler() = default;
};