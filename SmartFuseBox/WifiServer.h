#pragma once

#include <WiFiS3.h>
#include <SerialCommandManager.h>
#include "LoggingSupport.h"
#include "SharedConstants.h"

enum class WifiMode : uint8_t
{
    AccessPoint = 0,
    Client = 1
};

enum class WifiConnectionState : uint8_t
{
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Failed = 3
};

class WifiServer : public SingleLoggerSupport
{
private:
    bool _serverActive;
    WiFiServer _server;
    WifiMode _mode;
    WifiConnectionState _connectionState;
    uint16_t _port;
    bool _initialized;
    
    // AP mode settings
    char _ssid[MaxSSIDLength];
    char _password[MaxWiFiPasswordLength];
    
    // Connection tracking
    unsigned long _lastConnectionAttempt;
    unsigned long _connectionStartTime;
    static constexpr unsigned long ConnectionRetryIntervalMs = 10000;
    static constexpr unsigned long ConnectionTimeoutMs = 10000;
    static constexpr unsigned long ConnectionCheckIntervalMs = 500;
    
    void handleClient(WiFiClient& client);
    void sendResponse(WiFiClient& client, int statusCode, const char* contentType, const String& body);
    void handleCommandRoute(WiFiClient& client, const String& path, const String& query);
    void handleDataRoute(WiFiClient& client);
    void send404(WiFiClient& client);
    String parseQueryParameter(const String& query, const String& paramName);
    void updateClientConnection();
    void startServer();
    
public:
    WifiServer(SerialCommandManager* commandMgrComputer, uint16_t port = 80);
    ~WifiServer();
    
    // Configuration methods
    void setAccessPointMode(const char* ssid, const char* password = nullptr);
    void setClientMode(const char* ssid, const char* password);
    
    // Initialization and lifecycle
    bool begin();
    void end();
    void update();
    
    // Status methods
    bool isConnected() const;
    bool isInitialized() const { return _initialized; }
    WifiConnectionState getConnectionState() const { return _connectionState; }
    WifiMode getMode() const { return _mode; }
    String getIPAddress() const;
    String getSSID() const;
    int getSignalStrength() const;
};