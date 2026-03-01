#pragma once

#include "WifiRadioBridge.h"
#include "IWifiRadio.h"
#include "LoggingSupport.h"
#include "SystemDefinitions.h"
#include "INetworkCommandHandler.h"
#include "WarningManager.h"
#include "MessageBus.h"

constexpr uint16_t MaximumRequestSize = 512;   // GET-only API, typical request ~200 bytes
constexpr uint8_t MaximumPathLength = 128;
constexpr uint8_t MaxConcurrentClients = 2;
constexpr uint8_t MaxPersistentClients = 1;

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
	WiFiServer _server;
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
		WiFiClient client;
		char request[MaximumRequestSize + 1];
		unsigned long startTime;
		unsigned long lastActivity;
		ClientHandlingState state;
		bool isPersistent;
	};

	ActiveClient _activeClients[MaxConcurrentClients];
	
	void sendResponse(WiFiClient& client, int statusCode, const char* contentType, const char* body, bool isPersistent);
	void send400(WiFiClient& client, bool isPersistent);
	void send404(WiFiClient& client, bool isPersistent);
	void updateClientConnection();
	void updateClientHandling();
	void processClientRequest(uint8_t index);
	void startServer();
	void stopServer();
	bool handleIndex(WiFiClient& client, bool isPersistent, const char* path);
	bool dispatchToHandler(WiFiClient& client, INetworkCommandHandler* handler, const char* path, const char* method, const char* query, bool isPersistent);
	void registerJsonVisitors(NetworkJsonVisitor** jsonVisitors, uint8_t jsonVisitorCount);

	// Multi-client helper functions
	int8_t findFreeClientSlot();
	uint8_t getPersistentClientCount();
	void cleanupClient(uint8_t index);
	void handleClientState(uint8_t index, unsigned long now);
	bool acceptNewClient(WiFiClient& client, unsigned long now);
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