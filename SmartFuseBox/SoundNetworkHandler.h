#pragma once

#include "INetworkCommandHandler.h"
#include "SoundController.h"
#include "SharedConstants.h"


class SoundNetworkHandler : public INetworkCommandHandler
{
private:
	SoundController* _soundController;

	// Helper to format JSON response
	void formatJsonResponse(char* buffer, size_t size, bool success, const char* message = nullptr);
	void formatSoundStatusJson(char* buffer, size_t size);

public:
	explicit SoundNetworkHandler(SoundController* soundController);
	const char* getRoute() const override { return "/api/sound"; }
	CommandResult handleRequest(const String& method,
		const String& cmd,
		StringKeyValue* params,
		uint8_t paramCount,
		char* responseBuffer,
		size_t bufferSize) override;
};