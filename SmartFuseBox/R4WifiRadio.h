#pragma once

#include "Local.h"

#if defined(WIFI_SUPPORT)

#include <WiFiS3.h>
#include "IWifiRadio.h"
#include "R4WifiClient.h"

class R4WifiRadio : public IWifiRadio
{
private:
    WiFiServer _server;

public:
    R4WifiRadio() : _server(80)
    {
    }

    bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) override
    {
        WiFi.config(ip, ip, subnet);

        if (password && strlen(password) > 0)
        {
            return WiFi.beginAP(ssid, password);
        }

        return WiFi.beginAP(ssid);
    }

    bool beginClient(const char* ssid, const char* password) override
    {
        WiFi.begin(ssid, password);
        return true;
    }

    void disconnect() override
    {
        WiFi.disconnect();
    }

    void end() override
    {
        WiFi.end();
    }

    WifiConnectionState status() override
    {
        int wifiStatus = WiFi.status();
        switch (wifiStatus)
        {
            case WL_CONNECTED:
                return WifiConnectionState::Connected;
            case WL_IDLE_STATUS:
            case WL_SCAN_COMPLETED:
                return WifiConnectionState::Connecting;
            case WL_NO_MODULE:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
                return WifiConnectionState::Failed;
            case WL_DISCONNECTED:
            default:
                return WifiConnectionState::Disconnected;
        }
    }

    int32_t rssi() override
    {
        return WiFi.RSSI();
    }

    IPAddress localIP() override
    {
        return WiFi.localIP();
    }

    bool hasModule() override
    {
        return WiFi.status() != WL_NO_MODULE;
    }

    void beginServer(uint16_t port) override
    {
        _server = WiFiServer(port);
        _server.begin();
    }

    void endServer() override
    {
        _server.end();
    }

    IWifiClient* available() override
    {
        WiFiClient client = _server.available();
        if (client)
        {
            return new R4WifiClient(client);
        }
        return nullptr;
    }

    IWifiClient* createClient() override
    {
        return new R4WifiClient();
    }
};

#endif