#pragma once

#include "INetworkCommandHandler.h"
#include "SensorController.h"


class SensorNetworkHandler : public INetworkCommandHandler
{
private:
    SensorController* _sensorController;

public:
    explicit SensorNetworkHandler(SensorController* sensorController);

    const char* getRoute() const override { return "/api/sensor"; }

    void formatStatusJson(char* buffer, size_t size) override;

    CommandResult handleRequest(const String& method,
        const String& command,
        StringKeyValue* params,
        uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) override;
};