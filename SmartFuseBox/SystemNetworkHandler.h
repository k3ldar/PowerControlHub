#pragma once

#include "INetworkCommandHandler.h"
#include "SystemDefinitions.h"
#include "WifiController.h"


class SystemNetworkHandler : public INetworkCommandHandler
{
private:
	WifiController* _wifiController;

public:
	explicit SystemNetworkHandler(WifiController* wifiController);

	const char* getRoute() const override { return "/api/system"; }

	void formatWifiStatusJson(WiFiClient* client) override;

	void formatStatusJson(char* buffer, size_t size);

	CommandResult handleRequest(const char* method,
		const char* cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};