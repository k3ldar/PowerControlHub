#pragma once

#include <Arduino.h>

#include "BaseCommandHandler.h"
#include "IWifiController.h"

// Forward declarations
class ConfigController;
class ConfigSyncManager;

#if defined(SD_CARD_SUPPORT)
class SdCardConfigLoader;
#endif

#if defined(MQTT_SUPPORT)
class MQTTConfigCommandHandler;
class MQTTController;
#endif

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	IWifiController* _wifiController;
	ConfigController* _configController;
	ConfigSyncManager* _configSyncManager;

#if defined(SD_CARD_SUPPORT)
	SdCardConfigLoader* _sdCardConfigLoader;
#endif

#if defined(MQTT_SUPPORT)
	MQTTConfigCommandHandler* _mqttConfigHandler;
	MQTTController* _mqttController;
#endif

public:
	explicit ConfigCommandHandler(
		IWifiController* wifiController, 
		ConfigController* configController);

	void setConfigSyncManager(ConfigSyncManager* syncManager);

#if defined(SD_CARD_SUPPORT)
	void setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader);
#endif

#if defined(MQTT_SUPPORT)
	void setMqttConfigHandler(MQTTConfigCommandHandler* mqttConfigHandler);
	void setMqttController(MQTTController* mqttController);
#endif

	bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
	const char* const* supportedCommands(size_t& count) const override;
};