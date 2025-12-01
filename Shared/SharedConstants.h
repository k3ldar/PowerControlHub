#pragma once

#include "Local.h"

constexpr uint8_t DefaultValue = 0xFF;

constexpr char SystemHeartbeatCommand[] = "F0";
constexpr char SystemInitialized[] = "F1";
constexpr char SystemFreeMemory[] = "F2";
constexpr char SystemCpuUsage[] = "F3";
constexpr char SystemBluetoothStatus[] = "F4";
constexpr char SystemWifiStatus[] = "F5";

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
constexpr char ConfigRenameBoat[] = "C3";
constexpr char ConfigRenameRelay[] = "C4";
constexpr char ConfigMapHomeButton[] = "C5";
constexpr char ConfigSetButtonColor[] = "C6";
constexpr char ConfigBoatType[] = "C7";
constexpr char ConfigSoundRelayId[] = "C8";
constexpr char ConfigSoundStartDelay[] = "C9";

#if defined(ARDUINO_UNO_R4)
constexpr char ConfigBluetoothEnable[] = "C10";
constexpr char ConfigWifiEnable[] = "C11";
constexpr char ConfigWifiMode[] = "C12";
constexpr char ConfigWifiSSID[] = "C13";
constexpr char ConfigWifiPassword[] = "C14";
constexpr char ConfigWifiPort[] = "C15";
#endif

constexpr char WarningsActive[] = "W0";
constexpr char WarningsList[] = "W1";
constexpr char WarningStatus[] = "W2";
constexpr char WarningsClear[] = "W3";
constexpr char WarningsAdd[] = "W4";

const char SensorTemperature[] = "S0";
const char SensorHumidity[] = "S1";
const char SensorBearing[] = "S2";
const char SensorDirection[] = "S3";
const char SensorSpeed[] = "S4";
const char SensorCompassTemp[] = "S5";
const char SensorWaterLevel[] = "S6";
const char SensorWaterPumpActive[] = "S7";
const char SensorHornActive[] = "S8";


constexpr char AckSuccess[] = "ok";
constexpr char ValueParamName[] = "v";


constexpr unsigned long HeartbeatIntervalMs = 1000;
constexpr unsigned long HeartbeatTimeoutMs = 3000;


constexpr uint8_t MaxUint8Value = 0xFF;
constexpr uint16_t MaxUint16Value = 0xFFFF;


constexpr char PercentSuffix[] = "%";
constexpr char NoValueText[] = "--";
constexpr char HexPrefix[] = "0x";


constexpr unsigned long SerialInitTimeoutMs = 300;

constexpr uint8_t RelayControllerNotInitialised = 1;


#if defined(ARDUINO_UNO_R4)
constexpr uint8_t MaxSSIDLength = 33; // 32 chars + null
constexpr uint8_t MaxWiFiPasswordLength = 65; // 64 chars + null
#endif

struct CommandResult {
    bool success;
    uint8_t status;  // Can be: error code, single value, OR bitmask (8 relays)

    // Bit manipulation helpers (only use when status is bitmask)
    bool isRelayOn(uint8_t index) const {
        return (status & (1 << index)) != 0;
    }

    static CommandResult ok()
    {
        return CommandResult{ true };
    }

    static CommandResult okStatus(uint8_t statusValue) {
        return CommandResult{ true, statusValue };
    }

    static CommandResult error(uint8_t errorCode) {
        return CommandResult{ false, errorCode };
    }
};