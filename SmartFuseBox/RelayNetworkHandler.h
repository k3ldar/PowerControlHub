#pragma once

#include "INetworkCommandHandler.h"
#include "RelayController.h"

/**
 * @brief WiFi/HTTP adapter for relay commands.
 * 
 * Exposes relay control via REST endpoints:
 * - GET  /api/relay       Get all relay states
 * - POST /api/relay/all   Turn all relays on/off
 * - GET  /api/relay/:id   Get specific relay state
 * - PUT  /api/relay/:id   Set specific relay state
 */
class RelayNetworkHandler : public INetworkCommandHandler
{
private:
    RelayController* _relayController;
    
public:
    explicit RelayNetworkHandler(RelayController* relayController);
    
    const char* getRoute() const override { return "/api/relay"; }
    
    void formatStatusJson(char* buffer, size_t size) override;

    CommandResult handleRequest(const String& method,
        const String& command,
        StringKeyValue* params,
        uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) override;
};