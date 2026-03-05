#pragma once

#include "Local.h"
#include "SharedBaseCommandHandler.h"
#include "BroadcastManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"
#include "WarningManager.h"

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

class SystemCommandHandler : public SharedBaseCommandHandler
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
};
