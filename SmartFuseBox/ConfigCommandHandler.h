/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <Arduino.h>

#include "BaseConfigCommandHandler.h"
#include "IWifiController.h"
#include "Local.h"

// Forward declarations
class ConfigController;
class MessageBus;

#if defined(CARD_CONFIG_LOADER)
class SdCardConfigLoader;
#endif

#if defined(MQTT_SUPPORT)
class MQTTConfigCommandHandler;
class MQTTController;
#endif

class ConfigCommandHandler : public virtual BaseCommandHandler, public BaseConfigCommandHandler
{
private:
	IWifiController* _wifiController;
	ConfigController* _configController;
	MessageBus* _messageBus;

#if defined(CARD_CONFIG_LOADER)
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

	void setMessageBus(MessageBus* messageBus);

#if defined(CARD_CONFIG_LOADER)
	void setSdCardConfigLoader(SdCardConfigLoader* sdCardConfigLoader);
#endif

#if defined(MQTT_SUPPORT)
	void setMqttConfigHandler(MQTTConfigCommandHandler* mqttConfigHandler);
	void setMqttController(MQTTController* mqttController);
#endif

	bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override;
	const char* const* supportedCommands(size_t& count) const override;
};