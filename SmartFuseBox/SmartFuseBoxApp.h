#pragma once

#include "Local.h"

#include <SerialCommandManager.h>
#include <SensorManager.h>

#include "SmartFuseBoxConstants.h"
#include "MessageBus.h"
#include "RelayController.h"
#include "SoundController.h"
#include "WarningManager.h"

#include "RelayCommandHandler.h"
#include "SoundCommandHandler.h"
#include "InterceptDebugCommandHandler.h"
#include "SensorCommandHandler.h"
#include "WarningCommandHandler.h"
#include "AckCommandHandler.h"
#include "SystemCommandHandler.h"
#include "ConfigCommandHandler.h"
#include "SchedulerCommandHandler.h"
#include "BluetoothRadioBridge.h"
#include "IBluetoothRadio.h"
#include "WifiController.h"
#include "IWifiController.h"
#include "ConfigController.h"
#include "ConfigSyncManager.h"
#include "SensorController.h"
#include "ConfigNetworkHandler.h"
#include "RelayNetworkHandler.h"
#include "SoundNetworkHandler.h"
#include "SystemNetworkHandler.h"
#include "SensorNetworkHandler.h"
#include "WarningNetworkHandler.h"
#include "SchedulerNetworkHandler.h"

#if defined(SD_CARD_SUPPORT)
#include "SdCardLogger.h"
#include "MicroSdDriver.h"
#include "SDCardConfigLoader.h"
#endif

#if defined(MQTT_SUPPORT)
#include "MQTTController.h"
#include "MQTTConfigCommandHandler.h"
#include "MQTTRelayHandler.h"
#include "MQTTSensorHandler.h"
#include "MQTTSystemHandler.h"
#endif

#if defined(LED_MANAGER)
#include "LedMatrixManager.h"
#endif

#include "BaseSensor.h"
#include "RemoteSensor.h"

class SmartFuseBoxApp
{
private:
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;

    MessageBus _messageBus;
    RelayController _relayController;
    SoundController _soundController;
    BroadcastManager _broadcastManager;
    WarningManager _warningManager;

    RelayCommandHandler _relayHandler;
    SoundCommandHandler _soundHandler;
    InterceptDebugHandler _interceptDebugHandler;
    SensorCommandHandler _sensorCommandHandler;
    WarningCommandHandler _warningCommandHandler;
    AckCommandHandler _ackHandler;
    SystemCommandHandler _systemCommandHandler;

    PlatformBluetoothRadio _bluetoothController;
    WifiController _wifiController;

    ConfigController _configController;
    ConfigSyncManager _configSyncManager;
    ConfigCommandHandler _configHandler;

    ConfigNetworkHandler _configNetworkHandler;
    RelayNetworkHandler _relayNetworkHandler;
    SoundNetworkHandler _soundNetworkHandler;
    WarningNetworkHandler _warningNetworkHandler;
    SystemNetworkHandler _systemNetworkHandler;
    SensorNetworkHandler* _sensorNetworkHandler;

#if defined(SD_CARD_SUPPORT)
    SdCardLogger _sdCardLogger;
#endif

    SensorManager* _sensorManager;
    SensorController* _sensorController;

#if defined(MQTT_SUPPORT)
    MQTTController _mqttController;
    MQTTConfigCommandHandler _mqttConfigHandler;
    MQTTRelayHandler _mqttRelayHandler;
    MQTTSensorHandler* _mqttSensorHandler;
    MQTTSystemHandler _mqttSystemHandler;
    unsigned long _nextRunMqttMs;
#endif

#if defined(LED_MANAGER)
    LedMatrixManager _ledManager;
#endif

#if defined(CARD_CONFIG_LOADER)
    SdCardConfigLoader _sdCardConfigLoader;
    static SmartFuseBoxApp* _instance;
    static void onSdCardReadyCallback(bool isNewCard);
#endif

    void configureWifiSupport(Config* config);
    void configureBluetoothSupport(Config* config);

public:
    SmartFuseBoxApp(SerialCommandManager* commandMgrComputer,
        SerialCommandManager* commandMgrLink,
        const uint8_t* relayPins, uint8_t relayCount);

    void setup(BaseSensorHandler** sensorHandlers, uint8_t sensorHandlerCount,
        RemoteSensor** remoteSensors, uint8_t remoteSensorCount);
    void loop();

    MessageBus* messageBus() { return &_messageBus; }
    BroadcastManager* broadcastManager() { return &_broadcastManager; }
    WarningManager* warningManager() { return &_warningManager; }
    SensorCommandHandler* sensorCommandHandler() { return &_sensorCommandHandler; }

    IWifiController* wifiController() 
    {
        return &_wifiController;
    }

    IBluetoothRadio* bluetoothController()
    {
        return &_bluetoothController;
    }

#if defined(SD_CARD_SUPPORT)
    SdCardLogger* sdCardLogger() { return &_sdCardLogger; }
#endif
};

