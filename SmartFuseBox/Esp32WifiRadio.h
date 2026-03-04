#pragma once

#include "Local.h"

#if defined(WIFI_SUPPORT) && defined(ESP32)

#include <WiFi.h>
#include "IWifiRadio.h"
#include "Esp32WifiClient.h"

class Esp32WifiRadio : public IWifiRadio
{
private:
    WiFiServer _server;

public:
    Esp32WifiRadio() : _server(80)
    {
    }

    bool beginAP(
        const char* ssid,
        const char* password,
        IPAddress ip,
        IPAddress subnet) override
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(ip, ip, subnet);

        if (password && strlen(password) > 0)
        {
            return WiFi.softAP(ssid, password);
        }

        return WiFi.softAP(ssid);
    }

    bool beginClient(const char* ssid, const char* password) override
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        return true;
    }

    void disconnect() override
    {
        WiFi.disconnect();
    }

    void end() override
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
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
            case WL_NO_SHIELD:
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
        return true;
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
            return new Esp32WifiClient(client);
        }
        return nullptr;
    }

    IWifiClient* createClient() override
    {
        return new Esp32WifiClient();
    }
};

#endif