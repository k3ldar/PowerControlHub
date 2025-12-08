#pragma once

#include <WiFiS3.h>
#include "LoggingSupport.h"
#include "SystemDefinitions.h"
#include "INetworkCommandHandler.h"
#include "WarningManager.h"

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

enum class ClientHandlingState : uint8_t
{
	Idle,
	ReadingRequest,
	ProcessingRequest,
	KeepAlive
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
	WarningManager* _warningManager;
	
	// AP mode settings
	char _ssid[MaxSSIDLength];
	char _password[MaxWiFiPasswordLength];
	char _ipAddress[MaxIpAddressLength];


	// Network command handlers
	INetworkCommandHandler** _handlers;
	uint8_t _handlerCount;

	// json visitors
	JsonVisitor** _jsonVisitors;
	uint8_t _jsonVisitorCount;

	// Connection tracking
	unsigned long _lastConnectionAttempt;
	unsigned long _connectionStartTime;
	uint8_t _consecutiveFailures;
	int8_t _lastRSSI;
	static constexpr unsigned long ConnectionRetryIntervalMs = 10000;
	static constexpr unsigned long ConnectionTimeoutMs = 10000;
	static constexpr unsigned long ConnectionCheckIntervalMs = 500;
	static constexpr unsigned long ClientReadTimeoutMs = 5000;
	static constexpr unsigned long RSSICheckIntervalMs = 5000;
	static constexpr unsigned long BackoffIntervalMs = 60000;
	static constexpr uint8_t MaxConsecutiveFailures = 3;
	unsigned long _lastRSSICheck;
	static constexpr unsigned long PersistentTimeoutMs = 30000;

	struct ActiveClient
	{
		WiFiClient client;
		String request;
		unsigned long startTime;
		unsigned long lastActivity;
		ClientHandlingState state;
		bool isPersistent;
	} _activeClient;
	
	void sendResponse(WiFiClient& client, int statusCode, const char* contentType, const String& body);
	void send400(WiFiClient& client);
	void send404(WiFiClient& client);
	String parseQueryParameter(const String& query, const String& paramName);
	void updateClientConnection();
	void updateClientHandling();
	void processClientRequest();
	void startServer();
	void stopServer();
	bool handleIndex(WiFiClient& client, bool isPersistent, const String& path);
	bool dispatchToHandler(WiFiClient& client, INetworkCommandHandler* handler, const String& path, const String& method, const String& query);
	void registerJsonVisitors(JsonVisitor** jsonVisitors, uint8_t jsonVisitorCount);
	
public:
	WifiServer(SerialCommandManager* commandMgrComputer, WarningManager* warningManager, uint16_t port,
		INetworkCommandHandler** handlers, uint8_t handlerCount,
		JsonVisitor** jsonVisitors, uint8_t jsonVisitorCount);
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
	String getIpAddress() const;
	String getSSID() const;
	int getSignalStrength() const;
};