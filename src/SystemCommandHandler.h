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

#include "Local.h"
#include "SharedBaseCommandHandler.h"
#include "BroadcastManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"
#include "WarningManager.h"
#include "BaseConfigCommandHandler.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardLogger.h"
#include "MicroSdDriver.h"
#endif

#if defined(WIFI_SUPPORT)
#include "WifiController.h"
#endif

#if defined(MQTT_SUPPORT)
#include "MQTTController.h"
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
#include "OtaManager.h"
#endif

class SystemCommandHandler : public virtual BaseCommandHandler, public SharedBaseCommandHandler, public BaseConfigCommandHandler
{
private:
#if defined(WIFI_SUPPORT)
    WifiController* _wifiController = nullptr;
#endif

#if defined(SD_CARD_SUPPORT)
    SdCardLogger* _sdCardLogger = nullptr;
#endif

#if defined(MQTT_SUPPORT)
    MQTTController* _mqttController = nullptr;
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
    OtaManager* _otaManager = nullptr;
#endif

public:
    explicit SystemCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager);
    ~SystemCommandHandler();

    bool handleCommand(SerialCommandManager* sender, const char*, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

#if defined(WIFI_SUPPORT)
    void setWifiController(WifiController* wifiController);
#endif

#if defined(SD_CARD_SUPPORT)
    void setSdCardLogger(SdCardLogger* sdCardLogger);
#endif

#if defined(MQTT_SUPPORT)
    void setMqttController(MQTTController* mqttController);
#endif

#if defined(OTA_AUTO_UPDATE) && defined(ESP32) && defined(WIFI_SUPPORT)
    void setOtaManager(OtaManager* otaManager);
#endif
};
