#pragma once

#include "IWifiRadio.h"
#include "NullWifiClient.h"

// Null implementation of IWifiRadio for boards without WiFi hardware.
// All methods return safe defaults or failure states.
// Allows WiFi subsystem code to compile and gracefully handle missing hardware at runtime.
class NullWifiRadio : public IWifiRadio
{
public:
    bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) override
    {
		(void)ssid;
        (void)password;
		(void)ip;
		(void)subnet;
        return false;
    }

    bool beginClient(
        const char* ssid,
        const char* password) override
    {
        (void)ssid;
		(void)password;
        return false;
    }

    void disconnect() override
    {
    }

    void end() override
    {
    }

    WifiConnectionState status() override
    {
        return WifiConnectionState::Disconnected;
    }

    int32_t rssi() override
    {
        return 0;
    }

    IPAddress localIP() override
    {
        return IPAddress(0, 0, 0, 0);
    }

    bool hasModule() override
    {
        return false;
    }

    void beginServer(uint16_t port) override
    {
        (void)port;
    }

    void endServer() override
    {
    }

    IWifiClient* available() override
    {
        return nullptr;
    }

    IWifiClient* createClient() override
    {
        return new NullWifiClient();
    }
};
