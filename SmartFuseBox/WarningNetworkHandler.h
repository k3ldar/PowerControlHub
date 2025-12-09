#pragma once


#include "INetworkCommandHandler.h"
#include "WarningManager.h"
#include "SystemDefinitions.h"


class WarningNetworkHandler : public INetworkCommandHandler
{
private:
	WarningManager* _warningManager;

public:
	explicit WarningNetworkHandler(WarningManager* warningManager);

	const char* getRoute() const override { return "/api/warning"; }

	void formatWifiStatusJson(WiFiClient* client) override;

	void formatStatusJson(char* buffer, size_t size);

	CommandResult handleRequest(const String& method,
		const String& cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};