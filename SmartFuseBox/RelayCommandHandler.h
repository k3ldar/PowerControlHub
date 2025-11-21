#pragma once
#include "BaseCommandHandler.h"
#include "ConfigManager.h"

enum class RelayResult : uint8_t
{
    Success = 0,
    InvalidIndex = 1,
    Reserved = 2
};


// internal message handlers
class RelayCommandHandler : public BaseCommandHandler
{
private:
    bool* _relayStatus;
    uint8_t* _relays;
    uint8_t _relayCount;
    uint8_t _reservedSoundRelay;
    SerialCommandManager* _commandMgrComputer;
    SerialCommandManager* _commandMgrLink;
public:
    RelayCommandHandler(SerialCommandManager* commandMgrComputer, SerialCommandManager* commandMgrLink, const uint8_t* relayPins, uint8_t totalRelays);
    ~RelayCommandHandler();
    bool handleCommand(SerialCommandManager* sender, const String command, const StringKeyValue params[], int paramCount) override;

    const String* supportedCommands(size_t& count) const override;

    void setup();
    RelayResult setRelayStatus(uint8_t relayIndex, bool isOn);
	uint8_t getRelayStatus(uint8_t relayIndex) const;
private:
    void broadcastRelayStatus(const String& cmd, const StringKeyValue* param = nullptr);
};

