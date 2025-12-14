#include "WifiServer.h"
#include "SystemFunctions.h"


constexpr char response400[] = "\"error\":\"Bad Request\",\"message\":\"The request will not process due to client error\"";
constexpr char response404[] = "\"error\":\"Not Found\",\"message\":\"The requested resource was not found\"";

WifiServer::WifiServer(SerialCommandManager* commandMgrComputer, WarningManager* warningManager, uint16_t port,
	INetworkCommandHandler** handlers, uint8_t handlerCount, NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount)
	:   SingleLoggerSupport(commandMgrComputer), 
		_serverActive(false),
		_server(port),
		_mode(WifiMode::AccessPoint),
		_connectionState(WifiConnectionState::Disconnected),
		_port(port),
		_initialized(false),
		_warningManager(warningManager),
		_handlers(handlers),
		_handlerCount(handlerCount),
		_jsonVisitors(nullptr),
		_jsonVisitorCount(0),
		_lastConnectionAttempt(0),
		_connectionStartTime(0),
		_consecutiveFailures(0),
		_lastRSSI(0),
		_lastRSSICheck(0)
{
	_ssid[0] = '\0';
	_password[0] = '\0';
	_activeClient.state = ClientHandlingState::Idle;
	_activeClient.lastActivity = 0;
	_activeClient.isPersistent = false;
	_activeClient.request[0] = '\0';
	registerJsonVisitors(jsonVisitors, jsonVisitorCount);
}

WifiServer::~WifiServer()
{
	end();
}

void WifiServer::setAccessPointMode(const char* ssid, const char* password, const char* ipAddress)
{
	_mode = WifiMode::AccessPoint;
	strncpy(_ssid, ssid, sizeof(_ssid) - 1);
	_ssid[sizeof(_ssid) - 1] = '\0';
	if (ipAddress != nullptr)
	{
		strncpy(_ipAddress, ipAddress, sizeof(_ipAddress) - 1);
		_ipAddress[sizeof(_ipAddress) - 1] = '\0';
	}
	else
	{
		_ipAddress[0] = '\0';
	}
	
	if (password != nullptr)
	{
		strncpy(_password, password, sizeof(_password) - 1);
		_password[sizeof(_password) - 1] = '\0';
	}
	else
	{
		_password[0] = '\0';
	}
}

void WifiServer::setClientMode(const char* ssid, const char* password)
{
	_mode = WifiMode::Client;
	strncpy(_ssid, ssid, sizeof(_ssid) - 1);
	_ssid[sizeof(_ssid) - 1] = '\0';
	strncpy(_password, password, sizeof(_password) - 1);
	_password[sizeof(_password) - 1] = '\0';
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
		IPAddress apIp;

		if (apIp.fromString(_ipAddress))
		{
			sendDebug(F("Access Point IP set"), F("WifiServer"));
		}
		else
		{
			sendDebug(F("Using default Access Point IP"), F("WifiServer"));
			apIp = IPAddress(192, 168, 4, 1);
		}

		IPAddress subnet(255, 255, 255, 0);
		WiFi.config(apIp, apIp, subnet);

		if (strlen(_password) > 0)
		{
			success = WiFi.beginAP(_ssid, _password);
		}
		else
		{
			success = WiFi.beginAP(_ssid);
		}
		
		if (success)
		{
			sendDebug(F("Access Point started."), F("WifiServer"));
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
		sendDebug(F("Starting WiFi client connection"), F("WifiServer"));
		
		WiFi.begin(_ssid, _password);
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
	if (WiFi.status() == WL_NO_MODULE)
	{
		sendError(F("WiFi module not detected"), F("WifiServer"));
		_connectionState = WifiConnectionState::Failed;
		return;
	}

	if (!_serverActive)
	{
		_server.begin();
		_serverActive = true;
		_initialized = true;
		
		sendDebug(F("HTTP server started"), F("WifiServer"));
	}
}

void WifiServer::stopServer()
{
	if (_serverActive)
	{
		_server.end();
		_serverActive = false;
		sendDebug(F("HTTP server stopped"), F("WifiServer"));
	}
}

void WifiServer::end()
{
	stopServer();
	
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
	if (_serverActive && isConnected())
	{
		updateClientHandling();
	}
}

void WifiServer::updateClientHandling()
{
	unsigned long now = millis();
	
	switch (_activeClient.state)
	{
		case ClientHandlingState::Idle:
		{
			// Check for new client connection
			WiFiClient client = _server.available();
			if (client)
			{
				sendDebug(F("New client connected"), F("WifiServer"));
				_activeClient.client = client;
				_activeClient.request[0] = '\0';
				_activeClient.startTime = now;
				_activeClient.lastActivity = now;
				_activeClient.isPersistent = false;
				_activeClient.state = ClientHandlingState::ReadingRequest;
			}
			break;
		}
		
		case ClientHandlingState::ReadingRequest:
		{
			// Check for timeout first (before checking for new clients)
			if (SystemFunctions::hasElapsed(now, _activeClient.startTime, ClientReadTimeoutMs))
			{
				sendDebug(F("Client read timeout"), F("WifiServer"));
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
				break;
			}
			
			// Check if client is still connected
			if (!_activeClient.client.connected())
			{
				sendDebug(F("Client disconnected during read"), F("WifiServer"));
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
				break;
			}
			
			// Read available data (non-blocking)
			bool requestComplete = false;
			size_t requestLen = strlen(_activeClient.request);
			
			while (_activeClient.client.available())
			{
				char c = _activeClient.client.read();
				
				// Append character to buffer
				if (requestLen < MaximumRequestSize)
				{
					_activeClient.request[requestLen++] = c;
					_activeClient.request[requestLen] = '\0';
					_activeClient.lastActivity = now;  // Update activity timestamp
				}
				
				// Check for end of HTTP headers (\r\n\r\n)
				if (requestLen >= 4 && 
					_activeClient.request[requestLen - 4] == '\r' &&
					_activeClient.request[requestLen - 3] == '\n' &&
					_activeClient.request[requestLen - 2] == '\r' &&
					_activeClient.request[requestLen - 1] == '\n')
				{
					requestComplete = true;
					break;
				}
				
				// Safety check for request size
				if (requestLen >= MaximumRequestSize)
				{
					sendDebug(F("Request too large"), F("WifiServer"));
					send400(_activeClient.client);
					_activeClient.client.stop();
					_activeClient.state = ClientHandlingState::Idle;
					return;  // Exit early
				}
			}
			
			// Only check for concurrent connections AFTER attempting to read
			// This prevents interfering with the active connection
			WiFiClient newClient = _server.available();
			if (newClient)
			{
				sendDebug(F("Rejecting concurrent connection (reading)"), F("WifiServer"));
				newClient.println(F("HTTP/1.1 503 Service Unavailable"));
				newClient.println(F("Content-Type: text/plain"));
				newClient.println(F("Connection: close"));
				newClient.println(F("Retry-After: 2"));
				newClient.println();
				newClient.println(F("Server busy"));
				newClient.stop();
			}
			
			// Transition to processing if request is complete
			if (requestComplete)
			{
				_activeClient.state = ClientHandlingState::ProcessingRequest;
			}
			
			break;
		}
		
		case ClientHandlingState::ProcessingRequest:
		{
			WiFiClient newClient = _server.available();
			if (newClient)
			{
				sendDebug(F("Rejecting concurrent connection (processing)"), F("WifiServer"));
				newClient.println(F("HTTP/1.1 503 Service Unavailable"));
				newClient.println(F("Content-Type: text/plain"));
				newClient.println(F("Connection: close"));
				newClient.println(F("Retry-After: 2"));
				newClient.println();
				newClient.println(F("Server busy"));
				newClient.stop();
			}
			
			processClientRequest();
			break;
		}
		
		case ClientHandlingState::KeepAlive:
		{
			WiFiClient newClient = _server.available();
			if (newClient)
			{
				sendDebug(F("Rejecting concurrent connection (keepalive)"), F("WifiServer"));
				newClient.println(F("HTTP/1.1 503 Service Unavailable"));
				newClient.println(F("Content-Type: text/plain"));
				newClient.println(F("Connection: close"));
				newClient.println(F("Retry-After: 2"));
				newClient.println();
				newClient.println(F("Server busy"));
				newClient.stop();
			}
			
			// Check for timeout (30 seconds of inactivity)
			if (SystemFunctions::hasElapsed(now, _activeClient.lastActivity, PersistentTimeoutMs))
			{
				sendDebug(F("Persistent connection timeout"), F("WifiServer"));
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
				break;
			}
			
			// Check if client is still connected
			if (!_activeClient.client.connected())
			{
				sendDebug(F("Persistent client disconnected"), F("WifiServer"));
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
				break;
			}
			
			// Check for new data on the persistent connection
			if (_activeClient.client.available())
			{
				sendDebug(F("New request on persistent connection"), F("WifiServer"));
				_activeClient.request[0] = '\0';
				_activeClient.startTime = now;
				_activeClient.state = ClientHandlingState::ReadingRequest;
			}
			break;
		}
	}
}

void WifiServer::processClientRequest()
{
	if (_activeClient.request[0] == '\0')
	{
		return;
	}
	
	// Check for persistent connection header
	bool isPersistent = strstr(_activeClient.request, "X-Connection-Type: persistent") != nullptr;
	_activeClient.isPersistent = isPersistent;
	
	// Parse the request line (GET /path?query HTTP/1.1)
	// Find first space (after method)
	char* firstSpace = strchr(_activeClient.request, ' ');
	if (!firstSpace)
	{
		send404(_activeClient.client);
		if (!isPersistent)
		{
			_activeClient.client.stop();
			_activeClient.state = ClientHandlingState::Idle;
		}
		else
		{
			_activeClient.state = ClientHandlingState::KeepAlive;
			_activeClient.lastActivity = millis();
		}
		return;
	}
	
	// Extract method
	size_t methodLen = firstSpace - _activeClient.request;
	char method[8];  // Enough for "DELETE" + null
	if (methodLen >= sizeof(method))
	{
		send404(_activeClient.client);
		if (!isPersistent)
		{
			_activeClient.client.stop();
			_activeClient.state = ClientHandlingState::Idle;
		}
		else
		{
			_activeClient.state = ClientHandlingState::KeepAlive;
			_activeClient.lastActivity = millis();
		}
		return;
	}
	strncpy(method, _activeClient.request, methodLen);
	method[methodLen] = '\0';
	
	// Find second space (before HTTP version)
	char* secondSpace = strchr(firstSpace + 1, ' ');
	if (!secondSpace)
	{
		send404(_activeClient.client);
		if (!isPersistent)
		{
			_activeClient.client.stop();
			_activeClient.state = ClientHandlingState::Idle;
		}
		else
		{
			_activeClient.state = ClientHandlingState::KeepAlive;
			_activeClient.lastActivity = millis();
		}
		return;
	}
	
	// Only support GET for now
	if (strcmp(method, "GET") != 0)
	{
		send404(_activeClient.client);
		if (!isPersistent)
		{
			_activeClient.client.stop();
			_activeClient.state = ClientHandlingState::Idle;
		}
		else
		{
			_activeClient.state = ClientHandlingState::KeepAlive;
			_activeClient.lastActivity = millis();
		}
		return;
	}
	
	// Extract full path (between first and second space)
	size_t fullPathLen = secondSpace - (firstSpace + 1);
	char fullPath[128];  // Buffer for path+query
	if (fullPathLen >= sizeof(fullPath))
	{
		send404(_activeClient.client);
		if (!isPersistent)
		{
			_activeClient.client.stop();
			_activeClient.state = ClientHandlingState::Idle;
		}
		else
		{
			_activeClient.state = ClientHandlingState::KeepAlive;
			_activeClient.lastActivity = millis();
		}
		return;
	}
	strncpy(fullPath, firstSpace + 1, fullPathLen);
	fullPath[fullPathLen] = '\0';
	
	// Split path and query string at '?'
	char* queryStart = strchr(fullPath, '?');
	char path[64];
	char query[64];
	
	if (queryStart)
	{
		// Split into path and query
		size_t pathLen = queryStart - fullPath;
		if (pathLen >= sizeof(path))
		{
			send404(_activeClient.client);
			if (!isPersistent)
			{
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
			}
			else
			{
				_activeClient.state = ClientHandlingState::KeepAlive;
				_activeClient.lastActivity = millis();
			}
			return;
		}
		strncpy(path, fullPath, pathLen);
		path[pathLen] = '\0';
		
		// Copy query (skip the '?')
		strncpy(query, queryStart + 1, sizeof(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	else
	{
		// No query string
		strncpy(path, fullPath, sizeof(path) - 1);
		path[sizeof(path) - 1] = '\0';
		query[0] = '\0';
	}
	
	sendDebug(F("Processing request"), F("WifiServer"));
	bool handled = false;
	
	if (SystemFunctions::startsWith(path, F("/api/index")))
	{
		handleIndex(_activeClient.client, _activeClient.isPersistent, path);
		handled = true;
	}
	
	if (!handled && _handlers && _handlerCount > 0)
	{
		// Loop through handlers and find matching route
		for (size_t i = 0; i < _handlerCount; i++)
		{
			if (_handlers[i] && SystemFunctions::startsWith(path, _handlers[i]->getRoute()))
			{
				handled = dispatchToHandler(_activeClient.client, _handlers[i], path, method, query);
				break;
			}
		}
	}
	
	if (!handled)
	{
		send404(_activeClient.client);
	}
	
	// Decide next state based on connection type
	if (!isPersistent)
	{
		_activeClient.client.stop();
		sendDebug(F("Client disconnected (transient)"), F("WifiServer"));
		_activeClient.state = ClientHandlingState::Idle;
	}
	else
	{
		_activeClient.request[0] = '\0';  // Clear request buffer for next request
		_activeClient.lastActivity = millis();
		_activeClient.state = ClientHandlingState::KeepAlive;
		sendDebug(F("Client kept alive (persistent)"), F("WifiServer"));
	}
}

void WifiServer::send400(WiFiClient& client)
{
	sendResponse(client, 400, "application/json", response400);
}

void WifiServer::send404(WiFiClient& client)
{
	sendResponse(client, 404, "application/json", response404);
}

void WifiServer::sendResponse(WiFiClient& client, int statusCode, const char* contentType, const char* body)
{
	client.print(F("HTTP/1.1 "));
	client.print(statusCode);
	client.print(F(" "));
	
	switch (statusCode)
	{
		case 200:
			client.println(F("OK"));
			break;
		case 400:
			client.println(F("Bad Request"));
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
	
	// Conditionally set Connection header based on persistent flag
	if (_activeClient.isPersistent)
	{
		client.println(F("Connection: keep-alive"));
		client.print(F("Keep-Alive: timeout="));
		client.println(PersistentTimeoutMs / 1000);  // Send timeout in seconds
	}
	else
	{
		client.println(F("Connection: close"));
	}
	
	client.print(F("Content-Length: "));
	client.println(SystemFunctions::calculateLength(body) + 2); // 2 = {}
	client.println();
	client.print(F("{"));
	client.print(body);
	client.print(F("}"));
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

bool WifiServer::getIpAddress(char* buffer, const uint8_t bufferLength) const
{
	if (!buffer || bufferLength == 0) {
		return false;
	}

	if (!isConnected()) {
		return false;
	}

	// Copy the stored IP address or get from WiFi
	strncpy(buffer, _ipAddress, bufferLength - 1);
	buffer[bufferLength - 1] = '\0';
	return true;
}

bool WifiServer::getSSID(char* buffer, const uint8_t bufferLength) const
{
	if (!buffer || bufferLength == 0) {
		return false;
	}

	if (!isConnected()) {
		return false;
	}

	strncpy(buffer, _ssid, bufferLength - 1);
	buffer[bufferLength - 1] = '\0';
	return true;
}

int WifiServer::getSignalStrength() const
{
	if (_mode != WifiMode::Client || !isConnected())
	{
		return 0;
	}
	
	return WiFi.RSSI();
}

bool WifiServer::handleIndex(WiFiClient& client, bool isPersistent, const char* path)
{
	sendDebug(path, F("WifiServer"));

	// Send HTTP headers first
	client.print(F("HTTP/1.1 200 OK\r\n"));
	client.print(F("Content-Type: application/json\r\n"));

	if (isPersistent)
	{
		client.print(F("Connection: keep-alive\r\n"));
		client.print(F("Keep-Alive: timeout="));
		client.print(PersistentTimeoutMs / 1000);  // Send timeout in seconds
		client.print(F("\r\n"));
	}
	else
	{
		client.print(F("Connection: close\r\n"));
	}

	client.print(F("\r\n"));

	// Stream JSON response
	client.print(F("{"));


	bool firstEntry = true;

	for (uint8_t i = 0; i < _jsonVisitorCount; i++)
	{
		if (_jsonVisitors[i])
		{
			// Add comma separator (except before first entry)
			if (!firstEntry)
			{
				client.print(F(","));
			}

			_jsonVisitors[i]->formatWifiStatusJson(&client);
			firstEntry = false;
		}
	}

	client.print(F("}"));

	return true;
}

bool WifiServer::dispatchToHandler(WiFiClient& client, INetworkCommandHandler* handler, const char* path, const char* method, const char* query)
{
	if (!handler)
	{
		return false;
	}

	// Extract command from path: /api/{handler}/{command}
	// For example: /api/sound/H10 -> command = H10
	//              /api/config/C8 -> command = C8
	char command[MaximumPathLength];
	command[0] = '\0';

	sendDebug(F("dispatchToHandler"), F("WifiServer"));
	sendDebug(F("Path: "), F("WifiServer"));
	sendDebug(path, F("WifiServer"));
	sendDebug(F("Route: "), F("WifiServer"));
	sendDebug(handler->getRoute(), F("WifiServer"));
	sendDebug(F("Command: "), F("WifiServer"));
	sendDebug(command, F("WifiServer"));
	sendDebug(F("Query: "), F("WifiServer"));
	sendDebug(query, F("WifiServer"));
	
	size_t routeLen = SystemFunctions::calculateLength(handler->getRoute());
	size_t pathLen = SystemFunctions::calculateLength(path);

	if (pathLen > routeLen)
	{
	    size_t commandLen = pathLen - routeLen;

	    if (commandLen >= sizeof(command))
	    {
	        sendDebug(F("Command too long"), F("WifiServer"));
	        return false;
	    }
	    
	    // Extract everything after the handler route
	    SystemFunctions::substr(command, sizeof(command), path, SystemFunctions::calculateLength(handler->getRoute()), MaximumPathLength);
	    
	    // Remove leading slash if present
	    if (SystemFunctions::startsWith(command, F("/")))
	    {
	        SystemFunctions::substr(command, sizeof(command), command, 1);
	    }
	}

	// Parse query string into parameter array (max 6 parameters)
	StringKeyValue params[MaximumParameterCount] = {};
	uint8_t paramCount = 0;
	uint16_t queryLength = SystemFunctions::calculateLength(query);

	if (queryLength > 0)
	{
		uint8_t startIdx = 0;

		while (paramCount < MaximumParameterCount && startIdx < queryLength)
		{
			// Find the next '&' or end of string
			int32_t ampIdx = SystemFunctions::indexOf(query, '&', startIdx);
			if (ampIdx == -1)
			{
				ampIdx = queryLength;
			}

			// Extract this parameter
			char param[DefaultMaxParamKeyLength];
			SystemFunctions::substr(param, sizeof(param), query, startIdx, ampIdx - startIdx);

			// Split on '='
			int32_t equalsIdx = SystemFunctions::indexOf(param, '=', 0);

			if (equalsIdx != -1)
			{
				SystemFunctions::substr(params[paramCount].key, sizeof(params[paramCount].key), param, 0, equalsIdx);
				SystemFunctions::substr(params[paramCount].value, sizeof(params[paramCount].value), param, equalsIdx + 1);
				paramCount++;
			}

			startIdx = ampIdx + 1;
		}
	}

	// Prepare response buffer
	char responseBuffer[MaximumJsonResponseBufferSize];
	responseBuffer[0] = '\0';

	// Call handler with modified body containing command and parameters
	CommandResult result = handler->handleRequest(method, command, params, paramCount, responseBuffer, sizeof(responseBuffer));

	// Send response based on result
	if (result.success)
	{
		sendResponse(client, 200, "application/json", responseBuffer);
		sendDebug(F("Handler success: "), F("WifiServer"));
		return true;
	}
	else
	{
		// Check if buffer has error message
		if (responseBuffer[0] != '\0')
		{
			sendResponse(client, 400, "application/json", responseBuffer);
			return true;
		}

		sendDebug(F("Handler error"), F("WifiServer"));
	}

	return false;
}

void WifiServer::updateClientConnection()
{
	wl_status_t status = static_cast<wl_status_t>(WiFi.status());
	unsigned long now = millis();
	
	switch (_connectionState)
	{
		case WifiConnectionState::Connecting:
			// Check connection status periodically
			if (SystemFunctions::hasElapsed(now, _lastConnectionAttempt, ConnectionCheckIntervalMs))
			{
				_lastConnectionAttempt = now;
				
				if (status == WL_CONNECTED)
				{
					_connectionState = WifiConnectionState::Connected;
					_consecutiveFailures = 0;
					_lastRSSI = WiFi.RSSI();
					IPAddress ip = WiFi.localIP();
					snprintf(_ipAddress, sizeof(_ipAddress), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

					sendDebug(F("WiFi connected!"), F("WifiServer"));
					
					// Start the HTTP server now that we're connected
					startServer();
				}
				else if (SystemFunctions::hasElapsed(now, _connectionStartTime, ConnectionTimeoutMs))
				{
					// Connection timeout
					_connectionState = WifiConnectionState::Failed;
					_consecutiveFailures++;
					sendError(F("WiFi connection timeout"), F("WifiServer"));
				}
			}
			break;
			
		case WifiConnectionState::Connected:
		{
			// Monitor for disconnection
			if (status != WL_CONNECTED)
			{
				_connectionState = WifiConnectionState::Disconnected;
				sendDebug(F("WiFi connection lost"), F("WifiServer"));
				stopServer();  // Stop server to clean up TCP state
				break;
			}
			
			// Check RSSI periodically to detect weak signal before disconnection
			if (SystemFunctions::hasElapsed(now, _lastRSSICheck, RSSICheckIntervalMs))
			{
				_lastRSSICheck = now;
				_lastRSSI = WiFi.RSSI();
				
				if (_lastRSSI < WeakSignalWarningRSSI)
				{
					if (_warningManager && !_warningManager->isWarningActive(WarningType::WeakWifiSignal))
					{
						_warningManager->raiseWarning(WarningType::WeakWifiSignal);
					}
				}
				else if (_warningManager && _warningManager->isWarningActive(WarningType::WeakWifiSignal))
				{
					_warningManager->clearWarning(WarningType::WeakWifiSignal);
				}
			}

			break;
		}
			
		case WifiConnectionState::Disconnected:
		case WifiConnectionState::Failed:
		{
			// Use exponential backoff after multiple failures
			unsigned long retryInterval = ConnectionRetryIntervalMs;
			if (_consecutiveFailures >= MaxConsecutiveFailures)
			{
				retryInterval = BackoffIntervalMs;
			}
			
			// Attempt reconnection after retry interval
			if (SystemFunctions::hasElapsed(now, _lastConnectionAttempt, retryInterval))
			{
				if (_consecutiveFailures >= MaxConsecutiveFailures)
				{
					sendDebug(F("WiFi reconnection attempt"), F("WifiServer"));
				}
				else
				{
					sendDebug(F("Attempting WiFi reconnection..."), F("WifiServer"));
				}
				
				// Ensure clean state before reconnection
				WiFi.disconnect();
				delay(100);  // Brief delay for hardware reset
				
				WiFi.begin(_ssid, _password);
				_connectionState = WifiConnectionState::Connecting;
				_connectionStartTime = now;
				_lastConnectionAttempt = now;
			}
			break;
		}
	}
}

void WifiServer::registerJsonVisitors(NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount)
{
	if (_jsonVisitors)
	{
		delete[] _jsonVisitors;
		_jsonVisitors = nullptr;
		_jsonVisitorCount = 0;
	}

	_jsonVisitorCount = jsonVisitorCount;
	_jsonVisitors = new NetworkJsonVisitor * [_jsonVisitorCount];

	for (uint8_t i = 0; i < _jsonVisitorCount; i++)
	{
		_jsonVisitors[i] = jsonVisitors[i];
	}
}