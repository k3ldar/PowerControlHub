#pragma once

#include "SharedBaseCommandHandler.h"
#include "BroadcastManager.h"
#include "SystemDefinitions.h"
#include "SystemFunctions.h"
#include "WarningManager.h"
#include "Local.h"

#if defined(ARDUINO_UNO_R4)
#include "SdCardLogger.h"
#include "WifiController.h"
#include "MicroSdDriver.h"
#include "MQTTController.h"
#endif

class SystemCommandHandler : public SharedBaseCommandHandler
{
private:
#if defined(ARDUINO_UNO_R4)
    WifiController* _wifiController = nullptr;
    SdCardLogger* _sdCardLogger = nullptr;
#if defined(MQQT_SUPPORT)
    MQTTController* _mqttController = nullptr;
#endif
#endif

public:
    explicit SystemCommandHandler(BroadcastManager* broadcaster, WarningManager* warningManager);
    ~SystemCommandHandler();

    bool handleCommand(SerialCommandManager* sender, const char*, const StringKeyValue params[], uint8_t paramCount) override;
    const char* const* supportedCommands(size_t& count) const override;

#if defined(ARDUINO_UNO_R4)
    void setWifiController(WifiController* wifiController);
    void setSdCardLogger(SdCardLogger* sdCardLogger);

#if defined(MQQT_SUPPORT)
    void setMqttController(MQTTController* mqttController);
#endif
#endif
};
