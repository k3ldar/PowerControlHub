#pragma once

#include "INetworkCommandHandler.h"
#include "SoundController.h"
#include "SystemDefinitions.h"


class SoundNetworkHandler : public INetworkCommandHandler
{
private:
	SoundController* _soundController;

public:
	explicit SoundNetworkHandler(SoundController* soundController);

	const char* getRoute() const override { return "/api/sound"; }

	void formatWifiStatusJson(WiFiClient* client) override;

	void formatStatusJson(char* buffer, size_t size);

	CommandResult handleRequest(const String& method,
		const String& cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};