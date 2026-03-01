#pragma once

#include <Arduino.h>

#include "Local.h"
#include "BaseCommandHandler.h"
#include "ConfigController.h"
#include "WifiController.h"
#include "ConfigSyncManager.h"
#include "SDCardConfigLoader.h"

// Forward declarations
class BluetoothController;
class ConfigSyncManager;
class SdCardConfigLoader;

#if defined(MQTT_SUPPORT)
class MQTTConfigCommandHandler;
class MQTTController;
#endif

class ConfigCommandHandler : public BaseCommandHandler
{
private:
	WifiController* _wifiController;
	ConfigController* _configController;
	ConfigSyncManager* _configSyncManager;
	SdCardConfigLoader* _sdCardConfigLoader;

#if defined(MQTT_SUPPORT)
	MQTTConfigCommandHandler* _mqttConfigHandler;
	MQTTController* _mqttController;
#endif

public:
	explicit ConfigCommandHandler(
		WifiController* wifiController, 
		ConfigController* configController);

	void setConfigSyncManager(ConfigSyncManager* syncManager);
	void setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader);

#if defined(MQTT_SUPPORT)
	void setMqttConfigHandler(MQTTConfigCommandHandler* mqttConfigHandler);
	void setMqttController(MQTTController* mqttController);
#endif

	bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
	const char* const* supportedCommands(size_t& count) const override;
};