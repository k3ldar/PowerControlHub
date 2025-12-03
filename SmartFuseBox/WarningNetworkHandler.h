#pragma once


#include "INetworkCommandHandler.h"
#include "WarningManager.h"
#include "SharedConstants.h"


class WarningNetworkHandler : public INetworkCommandHandler
{
private:
	WarningManager* _warningManager;

	// Helper to format JSON response
	void formatJsonResponse(char* buffer, size_t size, bool success, const char* message = nullptr);
	void formatWarningStatusJson(char* buffer, size_t size);

public:
	explicit WarningNetworkHandler(WarningManager* warningManager);
	const char* getRoute() const override { return "/api/warning"; }
	CommandResult handleRequest(const String& method,
		const String& cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};