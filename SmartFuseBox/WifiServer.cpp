#include "WifiServer.h"
#include "SharedFunctions.h"

constexpr uint16_t MaximumResponseBufferSize = 512;
constexpr uint16_t MaximumRequestSize = 1024;

WifiServer::WifiServer(SerialCommandManager* commandMgrComputer, WarningManager* warningManager, uint16_t port,
	INetworkCommandHandler** handlers, uint8_t handlerCount, JsonVisitor** jsonVisitors, uint8_t jsonVisitorCount)
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
	registerJsonVisitors(jsonVisitors, jsonVisitorCount);
}

WifiServer::~WifiServer()
{
	end();
}

void WifiServer::setAccessPointMode(const char* ssid, const char* password)
{
	_mode = WifiMode::AccessPoint;
	strncpy(_ssid, ssid, sizeof(_ssid) - 1);
	_ssid[sizeof(_ssid) - 1] = '\0';
	
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
		sendDebug(String(F("Initializing in Access Point mode: ")) + String(_ssid), F("WifiServer"));
		
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
		sendDebug(String(F("Starting WiFi client connection to: ")) + String(_ssid), F("WifiServer"));
		
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
		
		sendDebug(String(F("HTTP server started on port ")) + String(_port), F("WifiServer"));
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
				_activeClient.request = "";
				_activeClient.startTime = now;
				_activeClient.state = ClientHandlingState::ReadingRequest;
			}
			break;
		}
		
		case ClientHandlingState::ReadingRequest:
		{
			// Check for timeout
			if (SharedFunctions::hasElapsed(now, _activeClient.startTime, ClientReadTimeoutMs))
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
			while (_activeClient.client.available())
			{
				char c = _activeClient.client.read();
				_activeClient.request += c;
				
				// Check for end of HTTP headers
				if (_activeClient.request.endsWith("\r\n\r\n"))
				{
					_activeClient.state = ClientHandlingState::ProcessingRequest;
					break;
				}
			}
			
			// Add safety check in ReadingRequest state
			if (_activeClient.request.length() > MaximumRequestSize)
			{
				sendDebug(F("Request too large"), F("WifiServer"));
				send400(_activeClient.client);
				_activeClient.client.stop();
				_activeClient.state = ClientHandlingState::Idle;
				break;
			}
			break;
		}
		
		case ClientHandlingState::ProcessingRequest:
		{
			processClientRequest();
			_activeClient.client.stop();
			sendDebug(F("Client disconnected"), F("WifiServer"));
			_activeClient.state = ClientHandlingState::Idle;
			break;
		}
	}
}

void WifiServer::processClientRequest()
{
	if (_activeClient.request.length() == 0)
	{
		return;
	}
	
	// Parse the request line (GET /path?query HTTP/1.1)
	int firstSpace = _activeClient.request.indexOf(' ');
	int secondSpace = _activeClient.request.indexOf(' ', firstSpace + 1);
	
	if (firstSpace == -1 || secondSpace == -1)
	{
		send404(_activeClient.client);
		return;
	}
	
	String method = _activeClient.request.substring(0, firstSpace);

	// Only support GET for now
	if (method != "GET")
	{
		send404(_activeClient.client);
		return;
	}

	String fullPath = _activeClient.request.substring(firstSpace + 1, secondSpace);
	
	// Split path and query string
	String path = fullPath;
	String query = "";
	int queryIndex = fullPath.indexOf('?');
	if (queryIndex != -1)
	{
		path = fullPath.substring(0, queryIndex);
		query = fullPath.substring(queryIndex + 1);
	}
	
	sendDebug(String(F("Request: ")) + method + String(F(" ")) + path, F("WifiServer"));
	bool handled = false;

	if (path.startsWith(F("/api/index")))
	{
		handleIndex(_activeClient.client, path);
		handled = true;
	}

	if (!handled && _handlers && _handlerCount > 0)
	{
		// Loop through handlers and find matching route
		for (size_t i = 0; i < _handlerCount; i++)
		{
			if (_handlers[i] && path.startsWith(_handlers[i]->getRoute()))
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
}

void WifiServer::send400(WiFiClient& client)
{
	String response = "\"error\":\"Bad Request\",\"message\":\"The request will not process due to client error\"";
	sendResponse(client, 400, "application/json", response);
}

void WifiServer::send404(WiFiClient& client)
{
	String response = "\"error\":\"Not Found\",\"message\":\"The requested resource was not found\"";
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
	client.println(F("Connection: close"));
	client.print(F("Content-Length: "));
	client.println(body.length());
	client.println();
	client.println(F("{"));
	client.print(body);
	client.println(F("}"));
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

String WifiServer::getIpAddress() const
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
	return String(_ssid);
}

int WifiServer::getSignalStrength() const
{
	if (_mode != WifiMode::Client || !isConnected())
	{
		return 0;
	}
	
	return WiFi.RSSI();
}

bool WifiServer::handleIndex(WiFiClient& client, const String& path)
{
	sendDebug(String(F("HandleIndex: ")) + path, F("WifiServer"));

	// Send HTTP headers first
	_activeClient.client.print(F("HTTP/1.1 200 OK\r\n"));
	_activeClient.client.print(F("Content-Type: application/json\r\n"));
	_activeClient.client.print(F("Connection: close\r\n"));
	_activeClient.client.print(F("\r\n"));

	// Stream JSON response
	_activeClient.client.print(F("{"));

	bool firstEntry = true;  // Track if we've written any content yet

	for (uint8_t i = 0; i < _jsonVisitorCount; i++)
	{
		if (_jsonVisitors[i])
		{
			char buffer[MaximumResponseBufferSize];
			buffer[0] = '\0';

			_jsonVisitors[i]->formatStatusJson(buffer, MaximumResponseBufferSize);

			if (buffer[0] != '\0')
			{
				// Add comma before this entry (but not before the first entry)
				if (!firstEntry)
				{
					_activeClient.client.print(F(","));
				}

				_activeClient.client.print(buffer);
				firstEntry = false;
			}
		}
	}

	_activeClient.client.print(F("}"));

	return true;
}

bool WifiServer::dispatchToHandler(WiFiClient& client, INetworkCommandHandler* handler, const String& path, const String& method, const String& query)
{
	if (!handler)
	{
		return false;
	}

	sendDebug(String(F("dispatchToHandler: ")) + path, F("WifiServer"));
	
	// Extract command from path: /api/{handler}/{command}
	// For example: /api/sound/H10 -> command = H10
	//              /api/config/C8 -> command = C8
	String command = "";
	String handlerRoute = String(handler->getRoute());
	
	if (path.length() > handlerRoute.length())
	{
		// Extract everything after the handler route
		command = path.substring(handlerRoute.length());
		
		// Remove leading slash if present
		if (command.startsWith("/"))
		{
			command = command.substring(1);
		}
	}

	// Parse query string into parameter array (max 6 parameters)
	StringKeyValue params[MaximumParameterCount];
	uint8_t paramCount = 0;

	if (query.length() > 0)
	{
		uint8_t startIdx = 0;

		while (paramCount < MaximumParameterCount && startIdx < query.length())
		{
			// Find the next '&' or end of string
			int ampIdx = query.indexOf('&', startIdx);
			if (ampIdx == -1)
			{
				ampIdx = query.length();
			}

			// Extract this parameter
			String param = query.substring(startIdx, ampIdx);

			// Split on '='
			int equalsIdx = param.indexOf('=');
			if (equalsIdx != -1)
			{
				params[paramCount].key = param.substring(0, equalsIdx);
				params[paramCount].value = param.substring(equalsIdx + 1);
				paramCount++;
			}

			startIdx = ampIdx + 1;
		}
	}

	// Prepare response buffer
	char responseBuffer[MaximumResponseBufferSize];
	responseBuffer[0] = '\0';

	// Call handler with modified body containing command and parameters
	CommandResult result = handler->handleRequest(method, command, params, paramCount, responseBuffer, sizeof(responseBuffer));

	// Send response based on result
	if (result.success)
	{
		sendResponse(client, 200, "application/json", String(responseBuffer));
		sendDebug(String(F("Handler success: ")) + String(handler->getRoute()) + String(F("/")) + command, F("WifiServer"));
		return true;
	}
	else
	{
		// Check if buffer has error message
		if (responseBuffer[0] != '\0')
		{
			sendResponse(client, 400, "application/json", String(responseBuffer));
			return true;
		}

		sendDebug(String(F("Handler error: ")) + String(handler->getRoute()) + String(F("/")) + command, F("WifiServer"));
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
			if (SharedFunctions::hasElapsed(now, _lastConnectionAttempt, ConnectionCheckIntervalMs))
			{
				_lastConnectionAttempt = now;
				
				if (status == WL_CONNECTED)
				{
					_connectionState = WifiConnectionState::Connected;
					_consecutiveFailures = 0;
					_lastRSSI = WiFi.RSSI();
					sendDebug(String(F("WiFi connected! IP: ")) + WiFi.localIP().toString() + 
							 String(F(" RSSI: ")) + String(_lastRSSI) + String(F(" dBm")), 
							 F("WifiServer"));
					
					// Start the HTTP server now that we're connected
					startServer();
				}
				else if (SharedFunctions::hasElapsed(now, _connectionStartTime, ConnectionTimeoutMs))
				{
					// Connection timeout
					_connectionState = WifiConnectionState::Failed;
					_consecutiveFailures++;
					sendError(String(F("WiFi connection timeout (#")) + String(_consecutiveFailures) + 
							 String(F("). Status: ")) + String(status), F("WifiServer"));
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
			if (SharedFunctions::hasElapsed(now, _lastRSSICheck, RSSICheckIntervalMs))
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
			if (SharedFunctions::hasElapsed(now, _lastConnectionAttempt, retryInterval))
			{
				if (_consecutiveFailures >= MaxConsecutiveFailures)
				{
					sendDebug(String(F("WiFi reconnection attempt (")) + 
							 String(_consecutiveFailures) + String(F(" failures, using ")) + 
							 String(BackoffIntervalMs / 1000) + String(F("s backoff)...")), 
							 F("WifiServer"));
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

void WifiServer::registerJsonVisitors(JsonVisitor** jsonVisitors, uint8_t jsonVisitorCount)
{
	if (_jsonVisitors)
	{
		delete[] _jsonVisitors;
		_jsonVisitors = nullptr;
		_jsonVisitorCount = 0;
	}

	_jsonVisitorCount = jsonVisitorCount;
	_jsonVisitors = new JsonVisitor * [_jsonVisitorCount];

	for (uint8_t i = 0; i < _jsonVisitorCount; i++)
	{
		_jsonVisitors[i] = jsonVisitors[i];
	}
}