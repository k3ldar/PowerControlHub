#pragma once

#include <Arduino.h>
#include "SystemDefinitions.h"

class IWifiClient;

class IWifiRadio
{
public:
    virtual ~IWifiRadio() = default;

    virtual bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) = 0;

    virtual bool beginClient(
        const char* ssid,
        const char* password) = 0;

    virtual void disconnect() = 0;
    virtual void end() = 0;
    virtual WifiConnectionState status() = 0;
    virtual int32_t rssi() = 0;
    virtual IPAddress localIP() = 0;
    virtual bool hasModule() = 0;

    virtual void beginServer(uint16_t port) = 0;
    virtual void endServer() = 0;
    virtual IWifiClient* available() = 0;
    virtual IWifiClient* createClient() = 0;
};