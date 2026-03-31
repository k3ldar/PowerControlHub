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
#include "SensorConfigCommandHandler.h"
#include "WarningCommandHandler.h"
#include "AckCommandHandler.h"
#include "SystemCommandHandler.h"
#include "ConfigCommandHandler.h"
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

#include "SchedulerCommandHandler.h"
#include "ScheduleController.h"

#if defined(LED_MANAGER)
#include "LedMatrixManager.h"
#endif

#include "BaseSensor.h"
#include "RemoteSensor.h"
#include "SensorFactory.h"

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
    SensorConfigCommandHandler _sensorConfigHandler;
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
    BaseSensor** _factorySensors;
    uint8_t _factorySensorCount;

#if defined(MQTT_SUPPORT)
    MQTTController _mqttController;
    MQTTConfigCommandHandler _mqttConfigHandler;
    MQTTRelayHandler _mqttRelayHandler;
    MQTTSensorHandler* _mqttSensorHandler;
    MQTTSystemHandler _mqttSystemHandler;
    unsigned long _nextRunMqttMs;
#endif

	SchedulerCommandHandler _schedulerCommandHandler;
	SchedulerNetworkHandler _schedulerNetworkHandler;
	ScheduleController _scheduleController;

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
        SerialCommandManager* commandMgrLink);

    void setup(RemoteSensor** remoteSensors, uint8_t remoteSensorCount);
    void loop();

    MessageBus* messageBus() { return &_messageBus; }
    BroadcastManager* broadcastManager() { return &_broadcastManager; }
    WarningManager* warningManager() { return &_warningManager; }
    SensorCommandHandler* sensorCommandHandler() { return &_sensorCommandHandler; }
    RelayController* relayController() { return &_relayController; }

    IWifiController* wifiController() 
    {
        return &_wifiController;
    }

    IBluetoothRadio* bluetoothController()
    {
        return &_bluetoothController;
    }

#if defined(SD_CARD_SUPPORT)
    SdCardLogger* sdCardLogger()
    {
        return &_sdCardLogger;
    }
#endif
};

