// SmartFuseBox.h
#pragma once

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

#include "BluetoothController.h"
#include "WifiController.h"
#include "ConfigController.h"
#include "ConfigSyncManager.h"
#include "SensorController.h"

#include "ConfigNetworkHandler.h"
#include "RelayNetworkHandler.h"
#include "SoundNetworkHandler.h"
#include "SystemNetworkHandler.h"
#include "SensorNetworkHandler.h"
#include "WarningNetworkHandler.h"

#include "SdCardLogger.h"
#include "MicroSdDriver.h"

#if defined(MQTT_SUPPORT)
#include "MQTTController.h"
#include "MQTTConfigCommandHandler.h"
#include "MQTTRelayHandler.h"
#include "MQTTSensorHandler.h"
#include "MQTTSystemHandler.h"
#endif

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
#include "LedMatrixManager.h"
#endif

#if defined(CARD_CONFIG_LOADER)
#include "SDCardConfigLoader.h"
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

    BluetoothController _bluetoothController;
    WifiController _wifiController;
    ConfigController _configController;
    ConfigSyncManager _configSyncManager;
    ConfigCommandHandler _configHandler;

    ConfigNetworkHandler _configNetworkHandler;
    RelayNetworkHandler _relayNetworkHandler;
    SoundNetworkHandler _soundNetworkHandler;
    WarningNetworkHandler _warningNetworkHandler;
    SystemNetworkHandler _systemNetworkHandler;

    SdCardLogger _sdCardLogger;

    SensorManager* _sensorManager;
    SensorController* _sensorController;
    SensorNetworkHandler* _sensorNetworkHandler;

#if defined(MQTT_SUPPORT)
    MQTTController _mqttController;
    MQTTConfigCommandHandler _mqttConfigHandler;
    MQTTRelayHandler _mqttRelayHandler;
    MQTTSensorHandler* _mqttSensorHandler;
    MQTTSystemHandler _mqttSystemHandler;
    unsigned long _nextRunMqttMs;
#endif

#if defined(ARDUINO_UNO_R4) && defined(LED_MANAGER)
    LedMatrixManager _ledManager;
#endif

#if defined(CARD_CONFIG_LOADER)
    SdCardConfigLoader _sdCardConfigLoader;
    static SmartFuseBox* _instance;
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
    WifiController* wifiController() { return &_wifiController; }
    BluetoothController* bluetoothController() { return &_bluetoothController; }
    SdCardLogger* sdCardLogger() { return &_sdCardLogger; }
};

