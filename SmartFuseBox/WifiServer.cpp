#include "WifiServer.h"

WifiServer::WifiServer(SerialCommandManager* commandMgrComputer, uint16_t port)
    : SingleLoggerSupport(commandMgrComputer), 
        _server(nullptr),
        _mode(WifiMode::AccessPoint),
        _connectionState(WifiConnectionState::Disconnected),
        _port(port),
        _initialized(false),
        _lastConnectionAttempt(0),
        _connectionStartTime(0)
{
    _apSSID[0] = '\0';
    _apPassword[0] = '\0';
    _clientSSID[0] = '\0';
    _clientPassword[0] = '\0';
}

WifiServer::~WifiServer()
{
    end();
}

void WifiServer::setAccessPointMode(const char* ssid, const char* password)
{
    _mode = WifiMode::AccessPoint;
    strncpy(_apSSID, ssid, sizeof(_apSSID) - 1);
    _apSSID[sizeof(_apSSID) - 1] = '\0';
    
    if (password != nullptr)
    {
        strncpy(_apPassword, password, sizeof(_apPassword) - 1);
        _apPassword[sizeof(_apPassword) - 1] = '\0';
    }
    else
    {
        _apPassword[0] = '\0';
    }
}

void WifiServer::setClientMode(const char* ssid, const char* password)
{
    _mode = WifiMode::Client;
    strncpy(_clientSSID, ssid, sizeof(_clientSSID) - 1);
    _clientSSID[sizeof(_clientSSID) - 1] = '\0';
    strncpy(_clientPassword, password, sizeof(_clientPassword) - 1);
    _clientPassword[sizeof(_clientPassword) - 1] = '\0';
}

bool WifiServer::begin()
{
    if (_initialized)
    {
        return true;
    }
    
    bool success = false;
    
    if (_mode == WifiMode::AccessPoint)
    {
        sendDebug(String(F("Initializing in Access Point mode: ")) + String(_apSSID), F("WifiServer"));
        
        if (strlen(_apPassword) > 0)
        {
            success = WiFi.beginAP(_apSSID, _apPassword);
        }
        else
        {
            success = WiFi.beginAP(_apSSID);
        }
        
        if (success)
        {
            sendDebug(String(F("Access Point started with IP: ")) + WiFi.localIP().toString(), F("WifiServer"));
            _connectionState = WifiConnectionState::Connected;
            startServer();
        }
        else
        {
            sendError(F("Failed to start Access Point"), F("WifiServer"));
            _connectionState = WifiConnectionState::Failed;
        }
    }
    else // Client mode - non-blocking initialization
    {
        sendDebug(String(F("Starting WiFi client connection to: ")) + String(_clientSSID), F("WifiServer"));
        
        WiFi.begin(_clientSSID, _clientPassword);
        _connectionState = WifiConnectionState::Connecting;
        _connectionStartTime = millis();
        _lastConnectionAttempt = _connectionStartTime;
        
        // Return true to indicate initialization started (not necessarily connected yet)
        success = true;
        _initialized = true;
    }
    
    return success;
}

void WifiServer::startServer()
{
    if (_server == nullptr)
    {
        _server = new WiFiServer(_port);
        _server->begin();
        _initialized = true;
        
        sendDebug(String(F("HTTP server started on port ")) + String(_port), F("WifiServer"));
    }
}

void WifiServer::end()
{
    if (_server != nullptr)
    {
        _server->end();
        delete _server;
        _server = nullptr;
    }
    
    if (_initialized)
    {
        WiFi.end();
        _initialized = false;
        _connectionState = WifiConnectionState::Disconnected;
    }
}

void WifiServer::update()
{
    if (!_initialized)
    {
        return;
    }
    
    // Handle client mode connection state machine
    if (_mode == WifiMode::Client)
    {
        updateClientConnection();
    }
    
    // Only handle HTTP clients if we're connected and server is running
    if (_server != nullptr && isConnected())
    {
        WiFiClient client = _server->available();
        if (client)
        {
            handleClient(client);
        }
    }
}

void WifiServer::updateClientConnection()
{
    wl_status_t status = static_cast<wl_status_t>(WiFi.status());
    unsigned long now = millis();
    
    switch (_connectionState)
    {
        case WifiConnectionState::Connecting:
            // Check connection status periodically
            if (now - _lastConnectionAttempt >= ConnectionCheckIntervalMs)
            {
                _lastConnectionAttempt = now;
                
                if (status == WL_CONNECTED)
                {
                    _connectionState = WifiConnectionState::Connected;
                    sendDebug(String(F("WiFi connected! IP: ")) + WiFi.localIP().toString() + 
                             String(F(" RSSI: ")) + String(WiFi.RSSI()) + String(F(" dBm")), 
                             F("WifiServer"));
                    
                    // Start the HTTP server now that we're connected
                    startServer();
                }
                else if (now - _connectionStartTime >= ConnectionTimeoutMs)
                {
                    // Connection timeout
                    _connectionState = WifiConnectionState::Failed;
                    sendError(String(F("WiFi connection timeout. Status: ")) + String(status), F("WifiServer"));
                }
            }
            break;
            
        case WifiConnectionState::Connected:
            // Monitor for disconnection
            if (status != WL_CONNECTED)
            {
                _connectionState = WifiConnectionState::Disconnected;
                sendDebug(F("WiFi connection lost"), F("WifiServer"));
            }
            break;
            
        case WifiConnectionState::Disconnected:
        case WifiConnectionState::Failed:
            // Attempt reconnection after retry interval
            if (now - _lastConnectionAttempt >= ConnectionRetryIntervalMs)
            {
                sendDebug(F("Attempting WiFi reconnection..."), F("WifiServer"));
                WiFi.begin(_clientSSID, _clientPassword);
                _connectionState = WifiConnectionState::Connecting;
                _connectionStartTime = now;
                _lastConnectionAttempt = now;
            }
            break;
    }
}

void WifiServer::handleClient(WiFiClient& client)
{
    sendDebug(F("New client connected"), F("WifiServer"));
    
    String request = "";
    unsigned long timeout = millis() + 5000;
    
    // Read the HTTP request
    while (client.connected() && millis() < timeout)
    {
        if (client.available())
        {
            char c = client.read();
            request += c;
            
            // Check for end of HTTP headers
            if (request.endsWith("\r\n\r\n"))
            {
                break;
            }
        }
    }
    
    if (request.length() == 0)
    {
        client.stop();
        return;
    }
    
    // Parse the request line (GET /path?query HTTP/1.1)
    int firstSpace = request.indexOf(' ');
    int secondSpace = request.indexOf(' ', firstSpace + 1);
    
    if (firstSpace == -1 || secondSpace == -1)
    {
        send404(client);
        client.stop();
        return;
    }
    
    String method = request.substring(0, firstSpace);
    String fullPath = request.substring(firstSpace + 1, secondSpace);
    
    // Split path and query string
    String path = fullPath;
    String query = "";
    int queryIndex = fullPath.indexOf('?');
    if (queryIndex != -1)
    {
        path = fullPath.substring(0, queryIndex);
        query = fullPath.substring(queryIndex + 1);
    }
    
    sendDebug(String(F("Request: ")) + fullPath, F("WifiServer"));
    
    // Route the request
    if (path.startsWith("/command"))
    {
        handleCommandRoute(client, path, query);
    }
    else if (path.startsWith("/data"))
    {
        handleDataRoute(client);
    }
    else
    {
        send404(client);
    }
    
    client.stop();
    sendDebug(F("Client disconnected"), F("WifiServer"));
}

void WifiServer::handleCommandRoute(WiFiClient& client, const String& path, const String& query)
{
    // Future work: integrate with command handlers
    // For now, return a placeholder response
    
    String cmd = parseQueryParameter(query, "cmd");
    String value = parseQueryParameter(query, "v");
    
    String response = "{\"status\":\"pending\",\"message\":\"Command handler integration not yet implemented\",\"command\":\"";
    response += cmd;
    response += "\",\"value\":\"";
    response += value;
    response += "\"}";
    
    sendResponse(client, 200, "application/json", response);
}

void WifiServer::handleDataRoute(WiFiClient& client)
{
    // Future work: collect data from sensors and system state
    // For now, return basic system information
    
    String json = "{";
    json += "\"uptime\":";
    json += millis();
    json += ",\"freeMemory\":";
    json += SharedFunctions::freeMemory();
    json += ",\"wifi\":{";
    json += "\"mode\":\"";
    json += (_mode == WifiMode::AccessPoint) ? "AP" : "Client";
    json += "\",\"state\":\"";
    
    // Add connection state
    switch (_connectionState)
    {
        case WifiConnectionState::Disconnected:
            json += "Disconnected";
            break;
        case WifiConnectionState::Connecting:
            json += "Connecting";
            break;
        case WifiConnectionState::Connected:
            json += "Connected";
            break;
        case WifiConnectionState::Failed:
            json += "Failed";
            break;
    }
    
    json += "\",\"ssid\":\"";
    json += getSSID();
    json += "\",\"ip\":\"";
    json += getIPAddress();
    json += "\"";
    
    if (_mode == WifiMode::Client && isConnected())
    {
        json += ",\"rssi\":";
        json += WiFi.RSSI();
    }
    
    json += "}";
    json += "}";
    
    sendResponse(client, 200, "application/json", json);
}

void WifiServer::send404(WiFiClient& client)
{
    String response = "{\"error\":\"Not Found\",\"message\":\"The requested resource was not found\"}";
    sendResponse(client, 404, "application/json", response);
}

void WifiServer::sendResponse(WiFiClient& client, int statusCode, const char* contentType, const String& body)
{
    client.print(F("HTTP/1.1 "));
    client.print(statusCode);
    client.print(F(" "));
    
    switch (statusCode)
    {
        case 200:
            client.println(F("OK"));
            break;
        case 404:
            client.println(F("Not Found"));
            break;
        default:
            client.println(F("Unknown"));
            break;
    }
    
    client.print(F("Content-Type: "));
    client.println(contentType);
    client.println(F("Connection: close"));
    client.print(F("Content-Length: "));
    client.println(body.length());
    client.println();
    client.print(body);
}

String WifiServer::parseQueryParameter(const String& query, const String& paramName)
{
    String searchFor = paramName + "=";
    int startIndex = query.indexOf(searchFor);
    
    if (startIndex == -1)
    {
        return "";
    }
    
    startIndex += searchFor.length();
    int endIndex = query.indexOf('&', startIndex);
    
    if (endIndex == -1)
    {
        return query.substring(startIndex);
    }
    
    return query.substring(startIndex, endIndex);
}

bool WifiServer::isConnected() const
{
    if (!_initialized)
    {
        return false;
    }
    
    if (_mode == WifiMode::AccessPoint)
    {
        return true; // AP is always "connected" once initialized
    }
    
    return _connectionState == WifiConnectionState::Connected;
}

String WifiServer::getIPAddress() const
{
    if (!_initialized)
    {
        return "0.0.0.0";
    }
    
    IPAddress ip = WiFi.localIP();
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

String WifiServer::getSSID() const
{
    if (_mode == WifiMode::AccessPoint)
    {
        return String(_apSSID);
    }
    else
    {
        return String(_clientSSID);
    }
}

int WifiServer::getSignalStrength() const
{
    if (_mode != WifiMode::Client || !isConnected())
    {
        return 0;
    }
    
    return WiFi.RSSI();
}


