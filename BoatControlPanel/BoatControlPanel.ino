#include "WarningType.h"
#include <Arduino.h>
#include <SerialCommandManager.h>
#include <NextionControl.h>

#include "BoatControlPanelConstants.h"
#include "BroadcastManager.h"
#include "InterceptDebugCommandHandler.h"
#include "AckCommandHandler.h"
#include "ConfigCommandHandler.h"
#include "SensorCommandHandler.h"
#include "WarningCommandHandler.h"
#include "SystemCommandHandler.h"

#include "HomePage.h"
#include "WarningPage.h"
#include "RelayPage.h"
#include "SoundSignalsPage.h"
#include "SoundOvertakingPage.h"
#include "SoundFogPage.h"
#include "SoundManeuveringPage.h"
#include "SoundEmergencyPage.h"
#include "SoundOtherPage.h"

#include "Config.h"
#include "ConfigManager.h"
#include "WarningManager.h"
#include "TLVCompass.h"


#define COMPUTER_SERIAL Serial
#define NEXTION_SERIAL Serial1
#define LINK_SERIAL Serial2


constexpr unsigned long UpdateIntervalMs = 600;
constexpr unsigned long SerialInitTimeoutMs = 300;
constexpr unsigned long HeartbeatIntervalMs = 1000;
constexpr unsigned long HeartbeatTimeoutMs = 3000;

// forward declares
void InitializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection = false);
void onLinkCommandReceived(SerialCommandManager* mgr);
void onComputerCommandReceived(SerialCommandManager* mgr);

// Compass with smoothing filter size 15
TLVCompass compass(15);

// Serial managers
SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', '=', 500, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

// Warning manager with heartbeat monitoring
WarningManager warningManager(&commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs);

// Nextion display setup
HomePage homePage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
WarningPage warningPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
RelayPage relayPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundSignalsPage soundSignalsPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundOvertakingPage soundOvertakingPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundFogPage soundFogPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundManeuveringPage soundManeuveringPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundEmergencyPage soundEmergencyPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundOtherPage soundOtherPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);

BaseDisplayPage* displayPages[] = { &homePage, &warningPage, &relayPage, &soundSignalsPage, &soundOvertakingPage,
    &soundFogPage, &soundManeuveringPage, &soundEmergencyPage, &soundOtherPage };
NextionControl nextion(&NEXTION_SERIAL, displayPages, sizeof(displayPages) / sizeof(displayPages[0]));

// link command handlers
InterceptDebugHandler interceptDebugHandler(&broadcastManager);
SensorCommandHandler sensorCommandHandler(&broadcastManager, &nextion, &warningManager);
WarningCommandHandler warningCommandHandler(&broadcastManager, &nextion, &warningManager);

// computer command handlers
ConfigCommandHandler configHandler(&broadcastManager, &homePage);

// shared command handlers
AckCommandHandler ackHandler(&broadcastManager, &nextion, &warningManager);
SystemCommandHandler systemCommandHandler(&broadcastManager);

// Timers
unsigned long lastUpdate = 0;
uint8_t speed = 0;

void setup()
{
    ISerialCommandHandler* linkHandlers[] = { &interceptDebugHandler, &ackHandler, &sensorCommandHandler, 
        &warningCommandHandler, &systemCommandHandler };
    size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
    commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

    ISerialCommandHandler* computerHandlers[] = { &configHandler, &ackHandler, &sensorCommandHandler, 
        &warningCommandHandler, &systemCommandHandler };
    size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
    commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

    InitializeSerial(COMPUTER_SERIAL, 115200, true);
    InitializeSerial(NEXTION_SERIAL, 19200);
    InitializeSerial(LINK_SERIAL, 9600, false);

    // retrieve config settings
    ConfigManager::begin();

    if (!ConfigManager::load())
    {
        warningManager.raiseWarning(WarningType::DefaultConfiguration);
    }

    Config* config = ConfigManager::getConfigPtr();
    homePage.configSet(config);
    warningPage.configSet(config);
	relayPage.configSet(config);

    nextion.begin();

    if (!compass.begin())
    {
        warningManager.raiseWarning(WarningType::SensorFailure);
        warningManager.raiseWarning(WarningType::CompassFailure);
    }

    // Simplified broadcasting
    broadcastManager.sendCommand(ConfigSoundRelayId, "v=" + String(config->hornRelayIndex));
    broadcastManager.sendCommand(ConfigBoatType, "v=" + String(static_cast<int>(config->vesselType)));
    broadcastManager.sendCommand(SystemInitialized, "");

	nextion.sendCommand(PageOne);
}

void loop()
{
    unsigned long now = millis();

    commandMgrComputer.readCommands();
    commandMgrLink.readCommands();

    nextion.update(now);
	warningManager.update(now);
	broadcastManager.update(now);

    if (now - lastUpdate >= UpdateIntervalMs)
    {
        lastUpdate = now;

        if (!warningManager.isWarningActive(WarningType::CompassFailure))
        {
            // Only update HomePage if it's the currently active page
            if (nextion.getCurrentPage() == &homePage)
            {
                if (speed > 40)
                    speed = 0;
                else
					speed += 2;

                homePage.setBearing(compass.getHeading());
                homePage.setDirection(compass.getDirection());
                homePage.setSpeed(speed);
                homePage.setCompassTemperature(compass.getTemperature());
            }
        }
    }
}

void onLinkCommandReceived(SerialCommandManager* mgr)
{
    String cmd = mgr->getCommand();
    commandMgrComputer.sendError(String(F("Unknown command: ")) + cmd, F("LINKHANDLER"));
}

void onComputerCommandReceived(SerialCommandManager* mgr)
{
    String cmd = mgr->getCommand();
    commandMgrComputer.sendError(String(F("Unknown command: ")) + cmd, F("PCHANDLER"));
}

void InitializeSerial(HardwareSerial& serialPort, unsigned long baudRate, bool waitForConnection)
{
    serialPort.begin(baudRate);

    if (waitForConnection)
    {
        unsigned long leave = millis() + SerialInitTimeoutMs;

        while (!serialPort && millis() < leave)
            delay(10);

        if (serialPort)
            delay(100);
    }
}
