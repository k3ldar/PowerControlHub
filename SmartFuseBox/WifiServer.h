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
#pragma once

#include "WifiRadioBridge.h"
#include "IWifiRadio.h"
#include "IWifiClient.h"
#include "LoggingSupport.h"
#include "SystemDefinitions.h"
#include "INetworkCommandHandler.h"
#include "WarningManager.h"
#include "MessageBus.h"
#include "Local.h"

constexpr uint16_t MaximumRequestSize = 512;   // GET-only API, typical request ~200 bytes
constexpr uint8_t MaximumPathLength = 128;

static constexpr unsigned long ConnectionRetryIntervalMs = 10000;
static constexpr unsigned long ConnectionTimeoutMs = 10000;
static constexpr unsigned long ConnectionCheckIntervalMs = 500;
static constexpr unsigned long ClientReadTimeoutMs = 2500;
static constexpr unsigned long RSSICheckIntervalMs = 5000;
static constexpr unsigned long BackoffIntervalMs = 60000;
static constexpr uint8_t MaxConsecutiveFailures = 3;

class WifiServer : public SingleLoggerSupport
{
private:
	MessageBus* _messageBus;
	bool _serverActive;
	WifiMode _mode;
	WifiConnectionState _connectionState;
	uint16_t _port;
	bool _initialized;
	WarningManager* _warningManager;

	// Network command handlers
	INetworkCommandHandler** _handlers;
	uint8_t _handlerCount;

	// json visitors
	NetworkJsonVisitor** _jsonVisitors;
	uint8_t _jsonVisitorCount;

	// Connection tracking
	unsigned long _lastConnectionAttempt;
	unsigned long _connectionStartTime;
	uint8_t _consecutiveFailures;
	int8_t _lastRSSI;
	unsigned long _lastRSSICheck;
	IWifiRadio* _radio;
	unsigned long _restartTime;
	// AP mode settings
	char _ssid[MaxSSIDLength];
	char _password[MaxWiFiPasswordLength];
	char _ipAddress[MaxIpAddressLength];


	static constexpr unsigned long PersistentTimeoutMs = 30000;

	struct ActiveClient
	{
		IWifiClient* client;
		char request[MaximumRequestSize + 1];
		unsigned long startTime;
		unsigned long lastActivity;
		ClientHandlingState state;
		bool isPersistent;
	};

	ActiveClient _activeClients[MaxConcurrentClients];

	void sendResponse(IWifiClient& client, int statusCode, const char* contentType, const char* body, bool isPersistent);
	void send400(IWifiClient& client, bool isPersistent);
	void send404(IWifiClient& client, bool isPersistent);
	void updateClientConnection();
	void updateClientHandling();
	void processClientRequest(uint8_t index);
	void startServer();
	void stopServer();
	bool handleIndex(IWifiClient& client, bool isPersistent, const char* path);
	bool dispatchToHandler(IWifiClient& client, INetworkCommandHandler* handler, const char* path, const char* method, const char* query, bool isPersistent);
	void registerJsonVisitors(NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount);

	// Multi-client helper functions
	int8_t findFreeClientSlot();
	uint8_t getPersistentClientCount();
	void cleanupClient(uint8_t index);
	void handleClientState(uint8_t index, unsigned long now);
	bool acceptNewClient(IWifiClient* client, unsigned long now);
	bool isStaticAssetRequest(const char* path);
	
public:
	WifiServer(MessageBus* messageBus, SerialCommandManager* commandMgrComputer,
		WarningManager* warningManager, uint16_t port,
		INetworkCommandHandler** handlers, uint8_t handlerCount,
		NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount,
		IWifiRadio* radio);
	~WifiServer();
	
	// Configuration methods
	void setAccessPointMode(const char* ssid, const char* password = nullptr, const char* ipAddress = nullptr);
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
	bool getIpAddress(char* buffer, const uint8_t bufferLength) const;
	bool getSSID(char* buffer, const uint8_t bufferLength) const;
	int getSignalStrength() const;
};