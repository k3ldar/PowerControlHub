/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Local.h"
#include "WifiServer.h"
#include "SystemFunctions.h"


constexpr char response400[] = "\"error\":\"Bad Request\",\"message\":\"The request will not process due to client error\"";
constexpr char response404[] = "\"error\":\"Not Found\",\"message\":\"The requested resource was not found\"";
constexpr uint8_t RestartDelay = 100;

WifiServer::WifiServer(MessageBus* messageBus, SerialCommandManager* commandMgrComputer, WarningManager* warningManager, uint16_t port,
	INetworkCommandHandler** handlers, uint8_t handlerCount, NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount, IWifiRadio* radio)
	:   SingleLoggerSupport(commandMgrComputer), 
		_messageBus(messageBus),
		_serverActive(false),
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
		_lastRSSICheck(0),
		_radio(radio)
{
	_ssid[0] = '\0';
	_password[0] = '\0';

	for (uint8_t i = 0; i < MaxConcurrentClients; i++)
	{
		_activeClients[i].client = nullptr;
		_activeClients[i].state = ClientHandlingState::Idle;
		_activeClients[i].lastActivity = 0;
		_activeClients[i].isPersistent = false;
		_activeClients[i].request[0] = '\0';
	}

	registerJsonVisitors(jsonVisitors, jsonVisitorCount);
}

WifiServer::~WifiServer()
{
	for (uint8_t i = 0; i < MaxConcurrentClients; i++)
	{
		if (_activeClients[i].client)
		{
			delete _activeClients[i].client;
			_activeClients[i].client = nullptr;
		}
	}
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
		success = _radio->beginAP(_ssid, _password, apIp, subnet);
		
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
		
		_radio->beginClient(_ssid, _password);
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
	if (!_radio->hasModule())
	{
		sendError(F("WiFi module not detected"), F("WifiServer"));
		_connectionState = WifiConnectionState::Failed;
		return;
	}

	if (!_serverActive)
	{
		_radio->beginServer(_port);
		_serverActive = true;
		_initialized = true;

		sendDebug(F("HTTP server started"), F("WifiServer"));
	}
}

void WifiServer::stopServer()
{
	if (_serverActive)
	{
		_radio->endServer();
		_serverActive = false;
		sendDebug(F("HTTP server stopped"), F("WifiServer"));
	}
}

void WifiServer::end()
{
	stopServer();

	if (_initialized)
	{
		_radio->end();
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

int8_t WifiServer::findFreeClientSlot()
{
	for (uint8_t i = 0; i < MaxConcurrentClients; i++)
	{
		if (_activeClients[i].state == ClientHandlingState::Idle)
		{
			return i;
		}
	}
	return -1;
}

uint8_t WifiServer::getPersistentClientCount()
{
	uint8_t count = 0;
	for (uint8_t i = 0; i < MaxConcurrentClients; i++)
	{
		if (_activeClients[i].isPersistent && 
			_activeClients[i].state != ClientHandlingState::Idle)
		{
			count++;
		}
	}
	return count;
}

void WifiServer::cleanupClient(uint8_t index)
{
	char msg[32];
	snprintf(msg, sizeof(msg), "Cleanup client [slot %d]", index);
	sendDebug(msg, F("WifiServer"));

	if (_activeClients[index].client)
	{
		_activeClients[index].client->stop();
		delete _activeClients[index].client;
		_activeClients[index].client = nullptr;
	}
	_activeClients[index].state = ClientHandlingState::Idle;
	_activeClients[index].isPersistent = false;
	_activeClients[index].request[0] = '\0';
}

bool WifiServer::isStaticAssetRequest(const char* path)
{
	if (path == nullptr)
	{
		return false;
	}

	// Find the last dot in the path
	const char* lastDot = strrchr(path, '.');
	if (lastDot == nullptr)
	{
		return false;
	}

	// Check for common browser static file requests
	// (This is a JSON-only API server, so reject these early with specific logging)
	const char* ext = lastDot + 1;
	return (strcmp(ext, "css") == 0 ||
			strcmp(ext, "js") == 0 ||
			strcmp(ext, "ico") == 0 ||      // favicon.ico
			strcmp(ext, "png") == 0 ||
			strcmp(ext, "jpg") == 0 ||
			strcmp(ext, "jpeg") == 0 ||
			strcmp(ext, "gif") == 0 ||
			strcmp(ext, "svg") == 0 ||
			strcmp(ext, "woff") == 0 ||     // web fonts
			strcmp(ext, "woff2") == 0 ||
			strcmp(ext, "ttf") == 0);
}

bool WifiServer::acceptNewClient(IWifiClient* client, unsigned long now)
{
	int8_t slot = findFreeClientSlot();
	if (slot < 0)
	{
		return false;
	}

	char msg[32];
	snprintf(msg, sizeof(msg), "Client connected [slot %d]", slot);
	sendDebug(msg, F("WifiServer"));

	_activeClients[slot].client = client;
	_activeClients[slot].request[0] = '\0';
	_activeClients[slot].startTime = now;
	_activeClients[slot].lastActivity = now;
	_activeClients[slot].isPersistent = false;
	_activeClients[slot].state = ClientHandlingState::ReadingRequest;

	return true;
}

void WifiServer::updateClientHandling()
{
	unsigned long now = millis();

	// Process all active clients
for (uint8_t i = 0; i < MaxConcurrentClients; i++)
	{
		if (_activeClients[i].state != ClientHandlingState::Idle)
		{
			handleClientState(i, now);
		}
	}

	// Check for new client connection
	IWifiClient* newClient = _radio->available();
	if (newClient)
	{
		if (!acceptNewClient(newClient, now))
		{
			// All slots full
			sendDebug(F("All connection slots full"), F("WifiServer"));
			newClient->println(F("HTTP/1.1 503 Service Unavailable"));
			newClient->println(F("Content-Type: text/plain"));
			newClient->println(F("Connection: close"));
			newClient->println(F("Retry-After: 5"));
			newClient->println();
			newClient->println(F("Server at capacity"));
			newClient->stop();
			delete newClient;
		}
	}
}

void WifiServer::handleClientState(uint8_t index, unsigned long now)
{
	ActiveClient& client = _activeClients[index];

	switch (client.state)
	{
		case ClientHandlingState::ReadingRequest:
		{
			// Check for timeout
			if (SystemFunctions::hasElapsed(now, client.startTime, ClientReadTimeoutMs))
			{
				sendDebug(F("Client read timeout"), F("WifiServer"));
				cleanupClient(index);
				break;
			}

			// Read available data (non-blocking)
			// Don't check connected() here - it can be false during TCP handshake
			// The timeout will catch truly dead connections
			bool requestComplete = false;
			size_t requestLen = strlen(client.request);

			while (client.client->available())
			{
				char c = client.client->read();

				// Append character to buffer
				if (requestLen < MaximumRequestSize)
				{
					client.request[requestLen++] = c;
					client.request[requestLen] = '\0';
					client.lastActivity = now;
				}

				// Safety check for request size
				if (requestLen >= MaximumRequestSize)
				{
					sendDebug(F("Request too large"), F("WifiServer"));
					send400(*client.client, false);
					cleanupClient(index);
					return;
				}
			}

			// Determine if the full request (headers + any body) has been received.
			// For GET: complete once we see the blank line (\r\n\r\n).
			// For POST: complete once Content-Length body bytes have also been read.
			const char* headerEnd = strstr(client.request, "\r\n\r\n");
			if (headerEnd)
			{
				size_t headerEndIdx = (headerEnd - client.request) + 4;
				int32_t contentLength = 0;
				const char* clHeader = strstr(client.request, "Content-Length:");
				if (clHeader)
				{
					clHeader += 15;
					while (*clHeader == ' ') clHeader++;
					contentLength = atoi(clHeader);
				}

				if (requestLen >= headerEndIdx + static_cast<size_t>(contentLength))
				{
					requestComplete = true;
					char msg[48];
					snprintf(msg, sizeof(msg), "Request complete [slot %d, %d bytes]", index, requestLen);
					sendDebug(msg, F("WifiServer"));
				}
			}

			// Transition to processing if request is complete
			if (requestComplete)
			{
				client.state = ClientHandlingState::ProcessingRequest;
			}

			break;
		}

		case ClientHandlingState::ProcessingRequest:
		{
			processClientRequest(index);
			break;
		}

		case ClientHandlingState::KeepAlive:
		{
			// Check for timeout
			if (SystemFunctions::hasElapsed(now, client.lastActivity, PersistentTimeoutMs))
			{
				sendDebug(F("Persistent connection timeout"), F("WifiServer"));
				cleanupClient(index);
				break;
			}

			// Check if still connected
			if (!client.client->connected())
			{
				sendDebug(F("Persistent client disconnected"), F("WifiServer"));
				cleanupClient(index);
				break;
			}

			// Check for new data on the persistent connection
			if (client.client->available())
			{
				sendDebug(F("New request on persistent connection"), F("WifiServer"));
				client.request[0] = '\0';
				client.startTime = now;
				client.state = ClientHandlingState::ReadingRequest;
			}
			break;
		}

		case ClientHandlingState::Idle:
			// Nothing to do
			break;
	}
}

void WifiServer::processClientRequest(uint8_t index)
{
	ActiveClient& client = _activeClients[index];

	if (client.request[0] == '\0')
	{
		return;
	}

	// Check for persistent connection header
	bool isPersistent = strstr(client.request, "X-Connection-Type: persistent") != nullptr;

	// Only allow persistent connections for authorized User-Agent
	if (isPersistent)
	{
		const char* userAgent = strstr(client.request, "User-Agent:");
		if (userAgent != nullptr)
		{
			// Move past "User-Agent: "
			userAgent += 12;

			// Skip leading whitespace
			while (*userAgent == ' ' || *userAgent == '\t')
			{
				userAgent++;
			}

			// Check if User-Agent matches "SmartFuseBox/1.0"
			if (strncmp(userAgent, "SmartFuseBox/1.0", 16) != 0)
			{
				// Not authorized for persistent connection
				isPersistent = false;
				sendDebug(F("Persistent denied (invalid UA)"), F("WifiServer"));
			}
			else
			{
				// Check if we already have max persistent connections
				if (getPersistentClientCount() >= MaxPersistentClients)
				{
					isPersistent = false;
					sendDebug(F("Persistent denied (quota full)"), F("WifiServer"));
				}
				else
				{
					sendDebug(F("Persistent authorized"), F("WifiServer"));
				}
			}
		}
		else
		{
			// No User-Agent header, deny persistent connection
			isPersistent = false;
			sendDebug(F("Persistent denied (no UA)"), F("WifiServer"));
		}
	}

	client.isPersistent = isPersistent;

	// Parse the request line (GET /path?query HTTP/1.1)
	// Find first space (after method)
	char* firstSpace = strchr(client.request, ' ');
	if (!firstSpace)
	{
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
		}
		return;
	}

	// Extract method
	size_t methodLen = firstSpace - client.request;
	char method[8];  // Enough for "DELETE" + null
	if (methodLen >= sizeof(method))
	{
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
		}
		return;
	}
	strncpy(method, client.request, methodLen);
	method[methodLen] = '\0';

	// Find second space (before HTTP version)
	char* secondSpace = strchr(firstSpace + 1, ' ');
	if (!secondSpace)
	{
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
		}
		return;
	}

	// Only support GET and POST
	if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0)
	{
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
		}
		return;
	}

	// Extract full path (between first and second space)
	size_t fullPathLen = secondSpace - (firstSpace + 1);
	char fullPath[128];  // Buffer for path+query
	if (fullPathLen >= sizeof(fullPath))
	{
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
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
			send404(*client.client, isPersistent);
			if (!isPersistent)
			{
				cleanupClient(index);
			}
			else
			{
				client.state = ClientHandlingState::KeepAlive;
				client.lastActivity = millis();
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

	// Early rejection of static asset requests (CSS, JS, images, etc.)
	if (isStaticAssetRequest(path))
	{
		sendDebug(F("Static asset request rejected"), F("WifiServer"));
		send404(*client.client, isPersistent);
		if (!isPersistent)
		{
			cleanupClient(index);
		}
		else
		{
			client.state = ClientHandlingState::KeepAlive;
			client.lastActivity = millis();
		}
		return;
	}

	if (_messageBus)
	{
		_messageBus->publish<WifiServerProcessingRequestChanged>(method, path, query, true);
	}
	sendDebug(F("Processing request"), F("WifiServer"));
	bool handled = false;

	// For POST requests, locate the body (bytes after the blank line \r\n\r\n)
	const char* body = nullptr;
	if (strcmp(method, "POST") == 0)
	{
		const char* bodyStart = strstr(client.request, "\r\n\r\n");
		if (bodyStart)
		{
			body = bodyStart + 4;
		}
	}

	if (SystemFunctions::startsWith(path, F("/api/index")))
	{
		handleIndex(*client.client, client.isPersistent, path);
		handled = true;
	}

	if (!handled && _handlers && _handlerCount > 0)
	{
		// Loop through handlers and find matching route
		for (size_t i = 0; i < _handlerCount; i++)
		{
			if (_handlers[i] && SystemFunctions::startsWith(path, _handlers[i]->getRoute()))
			{
				handled = dispatchToHandler(*client.client, _handlers[i], path, method, query, body, isPersistent);
				break;
			}
		}
	}

	if (!handled)
	{
		send404(*client.client, isPersistent);
	}

	// Decide next state based on connection type
	if (!isPersistent)
	{
		cleanupClient(index);
		sendDebug(F("Client disconnected (transient)"), F("WifiServer"));
	}
	else
	{
		client.request[0] = '\0';  // Clear request buffer for next request
		client.lastActivity = millis();
		client.state = ClientHandlingState::KeepAlive;
		sendDebug(F("Client kept alive (persistent)"), F("WifiServer"));
	}

	if (_messageBus)
	{
		_messageBus->publish<WifiServerProcessingRequestChanged>(method, path, query, false);
	}
}

void WifiServer::send400(IWifiClient& client, bool isPersistent)
{
	sendResponse(client, 400, "application/json", response400, isPersistent);
}

void WifiServer::send404(IWifiClient& client, bool isPersistent)
{
	sendResponse(client, 404, "application/json", response404, isPersistent);
}

void WifiServer::sendResponse(IWifiClient& client, int statusCode, const char* contentType, const char* body, bool isPersistent)
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
	if (isPersistent)
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
	if (!buffer || bufferLength == 0)
	{
		return false;
	}

	if (!isConnected())
	{
		return false;
	}

	// Copy the stored IP address or get from WiFi
	strncpy(buffer, _ipAddress, bufferLength - 1);
	buffer[bufferLength - 1] = '\0';
	return true;
}

bool WifiServer::getSSID(char* buffer, const uint8_t bufferLength) const
{
	if (!buffer || bufferLength == 0)
	{
		return false;
	}

	if (!isConnected())
	{
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
	
	return _radio->rssi();
}

bool WifiServer::handleIndex(IWifiClient& client, bool isPersistent, const char* path)
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

bool WifiServer::dispatchToHandler(IWifiClient& client, INetworkCommandHandler* handler, const char* path, const char* method, const char* query, const char* body, bool isPersistent)
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

	// Parse parameters:
	//   POST requests → newline-delimited key=value lines from the body
	//   GET  requests → '&'-delimited key=value pairs from the query string
	StringKeyValue params[MaximumParameterCount] = {};
	uint8_t paramCount = 0;

	if (strcmp(method, "POST") == 0 && body && body[0] != '\0')
	{
		// Body format supports two styles:
		//   Newline-delimited:  "key=value\nkey=value\n..."
		//   Semicolon-delimited: "key=value;key=value;..."  (serial/dat-file style)
		// Both styles are parsed identically — split on '\n', '\r', or ';'.
		const char* lineStart = body;

		while (paramCount < MaximumParameterCount && lineStart && *lineStart != '\0')
		{
			// Find end of token (newline or semicolon delimiter)
			const char* lineEnd = lineStart;
			while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r' && *lineEnd != ';') lineEnd++;

			size_t lineLen = lineEnd - lineStart;
			if (lineLen > 0 && lineLen < DefaultMaxParamKeyLength + DefaultMaxParamValueLength + 1)
			{
				char line[DefaultMaxParamKeyLength + DefaultMaxParamValueLength + 2];
				strncpy(line, lineStart, lineLen);
				line[lineLen] = '\0';

				int32_t eqIdx = SystemFunctions::indexOf(line, '=', 0);
				if (eqIdx > 0)
				{
					SystemFunctions::substr(params[paramCount].key,   sizeof(params[paramCount].key),   line, 0, eqIdx);
					SystemFunctions::substr(params[paramCount].value, sizeof(params[paramCount].value), line, eqIdx + 1);
					paramCount++;
				}
			}

			// Advance past delimiter(s)
			lineStart = lineEnd;
			while (*lineStart == '\r' || *lineStart == '\n' || *lineStart == ';') lineStart++;
		}
	}
	else
	{
		uint16_t queryLength = SystemFunctions::calculateLength(query);

		if (queryLength > 0)
		{
			uint8_t startIdx = 0;

			while (paramCount < MaximumParameterCount && startIdx < queryLength)
			{
				int32_t ampIdx = SystemFunctions::indexOf(query, '&', startIdx);
				if (ampIdx == -1)
				{
					ampIdx = queryLength;
				}

				char param[DefaultMaxParamKeyLength];
				SystemFunctions::substr(param, sizeof(param), query, startIdx, ampIdx - startIdx);

				int32_t equalsIdx = SystemFunctions::indexOf(param, '=', 0);
				if (equalsIdx != -1)
				{
					SystemFunctions::substr(params[paramCount].key,   sizeof(params[paramCount].key),   param, 0, equalsIdx);
					SystemFunctions::substr(params[paramCount].value, sizeof(params[paramCount].value), param, equalsIdx + 1);
					paramCount++;
				}

				startIdx = ampIdx + 1;
			}
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
		sendResponse(client, 200, "application/json", responseBuffer, isPersistent);
		sendDebug(F("Handler success: "), F("WifiServer"));
		return true;
	}
	else
	{
		// Check if buffer has error message
		if (responseBuffer[0] != '\0')
		{
			sendResponse(client, 400, "application/json", responseBuffer, isPersistent);
			return true;
		}

		sendDebug(F("Handler error"), F("WifiServer"));
	}

	return false;
}

void WifiServer::updateClientConnection()
{
	WifiConnectionState status = _radio->status();
	unsigned long now = millis();

	switch (_connectionState)
	{
		case WifiConnectionState::Connecting:
			// Check connection status periodically
			if (SystemFunctions::hasElapsed(now, _lastConnectionAttempt, ConnectionCheckIntervalMs))
			{
				_lastConnectionAttempt = now;

				if (status == WifiConnectionState::Connected)
				{
					_connectionState = WifiConnectionState::Connected;
					_consecutiveFailures = 0;
					_lastRSSI = _radio->rssi();
					IPAddress ip = _radio->localIP();
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
			if (status != WifiConnectionState::Connected)
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
				_lastRSSI = _radio->rssi();
				
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
				_radio->disconnect();
				_restartTime = now + RestartDelay;
				_connectionState = WifiConnectionState::Restarting;
			}
			break;
		}
		case WifiConnectionState::Restarting:
			if (now > _restartTime)
			{
				_radio->beginClient(_ssid, _password);
				_connectionState = WifiConnectionState::Connecting;
				_connectionStartTime = now;
				_lastConnectionAttempt = now;
			}

			break;
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