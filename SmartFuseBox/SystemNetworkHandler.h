#pragma once

#include "INetworkCommandHandler.h"
#include "SystemDefinitions.h"
#include "WifiController.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardLogger.h"
#endif


class SystemNetworkHandler : public INetworkCommandHandler
{
private:
	WifiController* _wifiController;

#if defined(SD_CARD_SUPPORT)
	SdCardLogger* _sdCardLogger;
#endif

public:
	explicit SystemNetworkHandler(WifiController* wifiController);

#if defined(SD_CARD_SUPPORT)
	void setSdCardLogger(SdCardLogger* sdCardLogger)
	{
		_sdCardLogger = sdCardLogger;
	}
#endif

	const char* getRoute() const override { return "/api/system"; }

	void formatWifiStatusJson(IWifiClient* client) override;

	void formatStatusJson(char* buffer, size_t size);

	CommandResult handleRequest(const char* method,
		const char* cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};