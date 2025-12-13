#pragma once

#include "INetworkCommandHandler.h"
#include "SystemDefinitions.h"


class ConfigNetworkHandler : public INetworkCommandHandler
{
public:
	explicit ConfigNetworkHandler();

	const char* getRoute() const override { return "/api/config"; }

	void formatWifiStatusJson(WiFiClient* client) override;

	void formatStatusJson(char* buffer, size_t size);

	CommandResult handleRequest(const char* method,
		const char* cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};