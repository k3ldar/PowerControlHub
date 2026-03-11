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

//#include "Local.h"

constexpr uint8_t DefaultValue = 0xFF;

constexpr uint8_t BufferSuccess = 0x00;
constexpr uint8_t BufferInvalid = 0x01;
constexpr uint8_t BufferOverflow = 0x02;
constexpr uint8_t InvalidConfiguration = 0x03;

constexpr char SystemHeartbeatCommand[] = "F0";
constexpr char SystemInitialized[] = "F1";
constexpr char SystemFreeMemory[] = "F2";
constexpr char SystemCpuUsage[] = "F3";
constexpr char SystemBluetoothStatus[] = "F4";
constexpr char SystemWifiStatus[] = "F5";
constexpr char SystemSetDateTime[] = "F6";
constexpr char SystemGetDateTime[] = "F7";
constexpr char SystemSdCardPresent[] = "F8";
constexpr char SystemSdCardLogFileSize[] = "F9";
constexpr char SystemRtcDiagnostic[] = "F10";
constexpr char SystemUptime[] = "F11";
constexpr char SystemPageDemo[] = "F12";

constexpr char RelayTurnAllOff[] = "R0";
constexpr char RelayTurnAllOn[] = "R1";
constexpr char RelayRetrieveStates[] = "R2";
constexpr char RelaySetState[] = "R3";
constexpr char RelayStatusGet[] = "R4";

constexpr char SoundSignalCancel[] = "H0";
constexpr char SoundSignalActive[] = "H1";
constexpr char SoundSignalSoS[] = "H2";
constexpr char SoundSignalFog[] = "H3";
constexpr char SoundSignalMoveStarboard[] = "H4";
constexpr char SoundSignalMovePort[] = "H5";
constexpr char SoundSignalMoveAstern[] = "H6";
constexpr char SoundSignalMoveDanger[] = "H7";
constexpr char SoundSignalOvertakeStarboard[] = "H8";
constexpr char SoundSignalOvertakePort[] = "H9";
constexpr char SoundSignalOvertakeConsent[] = "H10";
constexpr char SoundSignalOvertakeDanger[] = "H11";
constexpr char SoundSignalTest[] = "H12";


constexpr char ConfigSaveSettings[] = "C0";
constexpr char ConfigGetSettings[] = "C1";
constexpr char ConfigResetSettings[] = "C2";
constexpr char ConfigRename[] = "C3";
constexpr char ConfigRenameRelay[] = "C4";
constexpr char ConfigMapHomeButton[] = "C5";
constexpr char ConfigSetButtonColor[] = "C6";
constexpr char ConfigBoatType[] = "C7";
constexpr char ConfigSoundRelayId[] = "C8";
constexpr char ConfigSoundStartDelay[] = "C9";

constexpr char ConfigBluetoothEnable[] = "C10";

constexpr char ConfigWifiEnable[] = "C11";
constexpr char ConfigWifiMode[] = "C12";
constexpr char ConfigWifiSSID[] = "C13";
constexpr char ConfigWifiPassword[] = "C14";
constexpr char ConfigWifiPort[] = "C15";
constexpr char ConfigWifiState[] = "C16";
constexpr char ConfigWifiApIpAddress[] = "C17";

constexpr char ConfigDefaultRelayState[] = "C18";
constexpr char ConfigLinkRelays[] = "C19";
constexpr char ConfigTimeZoneOffset[] = "C20";
constexpr char ConfigMmsi[] = "C21";
constexpr char ConfigCallSign[] = "C22";
constexpr char ConfigHomePort[] = "C23";
constexpr char ConfigLedColor[] = "C24";
constexpr char ConfigLedBrightness[] = "C25";
constexpr char ConfigLedAutoSwitch[] = "C26";
constexpr char ConfigLedEnable[] = "C27";
constexpr char ControlPanelTones[] = "C28";
constexpr char ConfigReloadFromSd[] = "C29";
constexpr char ConfigExportToSd[] = "C30";
constexpr char ConfigSdCardSpeed[] = "C31";
constexpr char ConfigLightSensorNightRelay[] = "C32";
constexpr char ConfigLightSensorThreshold[] = "C33";

constexpr char MqttConfigEnable[] = "M0";
constexpr char MqttConfigBroker[] = "M1";
constexpr char MqttConfigPort[] = "M2";
constexpr char MqttConfigUsername[] = "M3";
constexpr char MqttConfigPassword[] = "M4";
constexpr char MqttConfigDeviceId[] = "M5";
constexpr char MqttConfigHADiscovery[] = "M6";
constexpr char MqttConfigKeepAlive[] = "M7";
constexpr char MqttConfigState[] = "M8";
constexpr char MqttConfigDiscoveryPrefix[] = "M9";

#if defined(SCHEDULER_SUPPORT)
constexpr char TimerListEvents[] = "T0";
constexpr char TimerGetEvent[] = "T1";
constexpr char TimerSetEvent[] = "T2";
constexpr char TimerDeleteEvent[] = "T3";
constexpr char TimerEnableEvent[] = "T4";
constexpr char TimerClearAll[] = "T5";
constexpr char TimerTriggerNow[] = "T6";
#endif

constexpr char WarningsActive[] = "W0";
constexpr char WarningsList[] = "W1";
constexpr char WarningStatus[] = "W2";
constexpr char WarningsClear[] = "W3";
constexpr char WarningsAdd[] = "W4";

constexpr char SensorTemperature[] = "S0";
constexpr char SensorHumidity[] = "S1";
constexpr char SensorBearing[] = "S2";
constexpr char SensorDirection[] = "S3";
constexpr char SensorSpeed[] = "S4";
constexpr char SensorCompassTemp[] = "S5";
constexpr char SensorWaterLevel[] = "S6";
constexpr char SensorWaterPumpActive[] = "S7";
constexpr char SensorHornActive[] = "S8";
constexpr char SensorLightSensor[] = "S9";
constexpr char SensorGpsLatLong[] = "S10";
constexpr char SensorGpsAltitude[] = "S11";
constexpr char SensorGpsSpeed[] = "S12";
constexpr char SensorGpsSatellites[] = "S13";
constexpr char SensorGpsDistance[] = "S14";


constexpr char AckSuccess[] = "ok";
constexpr char ValueParamName[] = "v";


constexpr unsigned long HeartbeatIntervalMs = 1000;
constexpr unsigned long HeartbeatTimeoutMs = 3000;


constexpr uint8_t ConfigMaxLinkedRelays = 2;


constexpr uint8_t MaxUint8Value = 0xFF;
constexpr uint16_t MaxUint16Value = 0xFFFF;
constexpr uint32_t MaxUint32Value = 0xFFFFFFFF;


constexpr char PercentSuffix[] = "%";
constexpr char NoValueText[] = "--";
constexpr char HexPrefix[] = "0x";

constexpr char CelsiusSuffix[] = "C";


constexpr unsigned long SerialInitTimeoutMs = 300;
constexpr uint16_t SerialPollingDelayMs = 10;
constexpr uint16_t SerialStabilizationDelayMs = 100;


constexpr uint8_t RelayControllerNotInitialised = 1;
constexpr uint8_t SoundControllerNotInitialised = 2;
constexpr uint8_t WarningControllerNotInitialised = 3;
constexpr uint8_t InvalidCommandParameters = 100;


constexpr uint8_t MaxSSIDLength = 33; // 32 chars + null
constexpr uint8_t MaxWiFiPasswordLength = 65; // 64 chars + null
constexpr uint8_t MaxIpAddressLength = 16; // xxx.xxx.xxx.xxx + null
constexpr char DefaultApIpAddress[MaxIpAddressLength] = "192.168.4.1";
constexpr uint16_t DefaultWifiPort = 80;


// WiFi Connection Quality Thresholds
constexpr int8_t WeakSignalWarningRSSI = -80;  // dBm - warn user

constexpr const char* compassDirections[16] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
};

constexpr uint16_t MaximumJsonResponseBufferSize = 512;

/*
* Unique IDs for different sensor types in the system.
* 
* These IDs are used to identify and manage various sensors, each sensor must have a unique ID.
* So for instance you could have two water sensors, but they would require a different WaterSensor ID.
*/
enum class SensorIdList : uint8_t
{
    WaterSensor = 0x0,
    Dht11Sensor = 0x1,
    LightSensor = 0x2,
    GpsSensor = 0x3,
    SystemSensor = 0x4
};

struct CommandResult {
    bool success;
    uint8_t status;

    // Bit manipulation helpers (only use when status is bitmask)
    bool isRelayOn(uint8_t index) const {
        return (status & (1 << index)) != 0;
    }

    static CommandResult ok()
    {
        return CommandResult{ true, 0 };
    }

    static CommandResult okStatus(uint8_t statusValue)
    {
        return CommandResult{ true, statusValue };
    }

    static CommandResult error(uint8_t errorCode)
    {
        return CommandResult{ false, errorCode };
    }
};


// Wifi server enums
enum class WifiMode : uint8_t
{
    AccessPoint = 0,
    Client = 1
};

enum class WifiConnectionState : uint8_t
{
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Failed = 3,
    Restarting = 4,
};

enum class ClientHandlingState : uint8_t
{
    Idle,
    ReadingRequest,
    ProcessingRequest,
    KeepAlive
};

constexpr uint16_t MillisecondsPerSecond = 1000;
constexpr uint8_t  SecondsPerMinute = 60;
constexpr uint8_t  MinutesPerHour = 60;
constexpr uint8_t  HoursPerDay = 24;
constexpr uint64_t Millis32BitOverflow = 0x100000000ULL;

constexpr uint8_t  GeneratedPasswordMinBufferSize = 15;
constexpr uint32_t PasswordRandomMin = 0x10000000;

constexpr uint8_t  LineBreakLength = 2;  // \r\n

