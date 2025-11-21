#pragma once


#include <Arduino.h>
#include "Config.h"
#include "ConfigManager.h"
#include "BaseCommandHandler.h"
#include "SoundManager.h"

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	SoundManager* _soundManager;

	void updateSoundManagerConfig(Config* config);
public:
	// Constructor: pass the HomePage pointer so we can notify UI when saved/updated
	explicit ConfigCommandHandler(SoundManager* soundManager);

	bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;
	const String* supportedCommands(size_t& count) const override;
};