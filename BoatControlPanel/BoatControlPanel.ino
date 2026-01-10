#include "Local.h"
#include <Arduino.h>
#include <SerialCommandManager.h>
#include <NextionControl.h>
#include <SensorManager.h>

#if defined(ARDUINO_R4_MINIMA)
#include <SoftwareSerial.h>
#endif

#include "BoatControlPanelConstants.h"
#include "BroadcastManager.h"
#include "InterceptDebugCommandHandler.h"
#include "AckCommandHandler.h"
#include "ConfigCommandHandler.h"
#include "SensorCommandHandler.h"
#include "WarningCommandHandler.h"
#include "SystemCommandHandler.h"
#include "SystemCpuMonitor.h"
#include "DateTimeManager.h"

#include "SplashPage.h"
#include "HomePage.h"
#include "WarningType.h"
#include "WarningPage.h"
#include "SystemPage.h"
#include "RelayPage.h"
#include "SoundSignalsPage.h"
#include "SoundOvertakingPage.h"
#include "SoundFogPage.h"
#include "SoundManeuveringPage.h"
#include "SoundEmergencyPage.h"
#include "SoundOtherPage.h"
#include "FlagsPage.h"
#include "CardinalMarkersPage.h"
#include "BuoysPage.h"
#include "AboutPage.h"

#include "Config.h"
#include "ConfigManager.h"
#include "WarningManager.h"

// sensors
//#include "TLVCompassHandler.h"

#include "GpsSensorHandler.h"
#include "SensorCommandHandler.h"

#if defined(ARDUINO_MEGA2560)
#define NEXTION_SERIAL Serial1
#define LINK_SERIAL Serial2
#define GPS_SERIAL Serial3
#elif defined(ARDUINO_R4_MINIMA)
SoftwareSerial LINK_SERIAL(2, 3); // RX, TX
SoftwareSerial NEXTION_SERIAL(4, 5); // RX, TX
#else
#error "You must define 'ARDUINO_MEGA2560' or 'ARDUINO_R4_MINIMA'"
#endif


#define COMPUTER_SERIAL Serial


constexpr unsigned long UpdateIntervalMs = 600;

// forward declares
void onLinkCommandReceived(SerialCommandManager* mgr);
void onComputerCommandReceived(SerialCommandManager* mgr);

// Serial managers
SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', ';', '=', 512, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', ';', '=', 1024, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

// Warning manager with heartbeat monitoring
WarningManager warningManager(&commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs);

// Nextion display setup
SplashPage splashPage(&NEXTION_SERIAL);
HomePage homePage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
WarningPage warningPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
RelayPage relayPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundSignalsPage soundSignalsPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundOvertakingPage soundOvertakingPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundFogPage soundFogPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundManeuveringPage soundManeuveringPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundEmergencyPage soundEmergencyPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SoundOtherPage soundOtherPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SystemPage systemPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
FlagsPage flagsPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
CardinalMarkersPage cardinalMarkersPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
BuoysPage buoysPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
AboutPage aboutPage(&NEXTION_SERIAL);

BaseDisplayPage* displayPages[] = { &splashPage, &homePage, &warningPage, &relayPage, &soundSignalsPage, 
    &soundOvertakingPage, &soundFogPage, &soundManeuveringPage, &soundEmergencyPage, &soundOtherPage,
    &systemPage, &flagsPage, & cardinalMarkersPage, &buoysPage, &aboutPage };
NextionControl nextion(&NEXTION_SERIAL, displayPages, sizeof(displayPages) / sizeof(displayPages[0]));

// link command handlers
InterceptDebugHandler interceptDebugHandler(&broadcastManager);
SensorCommandHandler sensorCommandHandler(&broadcastManager, &nextion, &warningManager);
WarningCommandHandler warningCommandHandler(&broadcastManager, &nextion, &warningManager);

// computer command handlers
ConfigCommandHandler configHandler(&broadcastManager, &homePage);

// shared command handlers
AckCommandHandler ackHandler(&broadcastManager, &nextion, &warningManager);
SystemCommandHandler systemCommandHandler(&broadcastManager, &warningManager);

// sensors
//TLVCompassHandler compass(&commandMgrComputer, 5);

GpsSensorHandler gpsSensor(&GPS_SERIAL, &broadcastManager, &sensorCommandHandler, &warningManager);

BaseSensor* baseSensors[] = {
    &gpsSensor
};
uint8_t sensorHandlerCount = sizeof(baseSensors) / sizeof(baseSensors[0]);
SensorManager sensorManager(baseSensors, sensorHandlerCount);


// Timers
unsigned long lastUpdate = 0;
uint8_t speed = 0;

void setup()
{
    DateTimeManager::setDateTime();

    ISerialCommandHandler* linkHandlers[] = { &interceptDebugHandler, &ackHandler, &sensorCommandHandler,
        &warningCommandHandler, &systemCommandHandler };
    size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
    commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

    ISerialCommandHandler* computerHandlers[] = { &configHandler, &ackHandler, &sensorCommandHandler, 
        &warningCommandHandler, &systemCommandHandler };
    size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
    commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

    SystemFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);

#if defined(ARDUINO_MEGA2560)
    SystemFunctions::initializeSerial(NEXTION_SERIAL, 19200, false);
    SystemFunctions::initializeSerial(LINK_SERIAL, 9600, false);
	SystemFunctions::initializeSerial(GPS_SERIAL, 9600, false);
#elif defined(ARDUINO_R4_MINIMA)
	NEXTION_SERIAL.begin(19200);
	LINK_SERIAL.begin(9600);
	GPS_SERIAL.begin(9600);
#endif

#if defined(NEXTION_DEBUG)
    nextion.setDebugCallback([](const String& msg) {
        commandMgrComputer.sendCommand(msg.c_str(), "NEXTION");
        });
#endif

    // retrieve config settings
    ConfigManager::begin();

    if (!ConfigManager::load())
    {
        warningManager.raiseWarning(WarningType::DefaultConfigurationControlPanel);
    }

    Config* config = ConfigManager::getConfigPtr();
    homePage.configSet(config);
    warningPage.configSet(config);
	relayPage.configSet(config);

    nextion.begin();
    
    sensorManager.setup();
    //commandMgrComputer.sendError(memBuffer, "MEMORY");

    //if (!compass.begin())
    //{
    //    snprintf(memBuffer, sizeof(memBuffer), "Compass failed: %d", freeMemory());
    //    commandMgrComputer.sendError(memBuffer, "MEMORY");
    //    
    //    warningManager.raiseWarning(WarningType::SensorFailure);
    //    warningManager.raiseWarning(WarningType::CompassFailure);
    //}

    // Simplified broadcasting
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "v=%u", config->hornRelayIndex);
    broadcastManager.sendCommand(ConfigSoundRelayId, buffer);
    snprintf(buffer, sizeof(buffer), "v=%u", static_cast<uint8_t>(config->vesselType));
    broadcastManager.sendCommand(ConfigBoatType, buffer);
    broadcastManager.sendCommand(SystemInitialized, "");

	nextion.sendCommand(PageOne);
}
void loop()
{
    unsigned long now = millis();

    SystemCpuMonitor::startTask();
    commandMgrComputer.readCommands();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    commandMgrLink.readCommands();
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    nextion.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    warningManager.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    broadcastManager.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    sensorManager.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    if (now - lastUpdate >= UpdateIntervalMs)
    {
        lastUpdate = now;

        commandMgrLink.sendCommand(WarningsList, "");

        if (!warningManager.isWarningActive(WarningType::CompassFailure))
        {
            // Only update HomePage if it's the currently active page
            if (nextion.getCurrentPage() == &homePage)
            {
                if (speed > 40)
                    speed = 0;
                else
					speed += 2;

                //homePage.setBearing(compass.getHeading());
                //homePage.setDirection(compass.getDirection());
                homePage.setSpeed(speed);
                //homePage.setCompassTemperature(compass.getTemperature());
            }
        }
    }

    SystemCpuMonitor::endTask();
    SystemCpuMonitor::update();
}

void onLinkCommandReceived(SerialCommandManager* mgr)
{
    char cmd[64];
	snprintf(cmd, sizeof(cmd), "%s", mgr->getCommand());
    commandMgrComputer.sendError(cmd, "LINKHANDLER");
}

void onComputerCommandReceived(SerialCommandManager* mgr)
{
    char cmd[64];
	snprintf(cmd, sizeof(cmd), "%s", mgr->getCommand());

    commandMgrComputer.sendError(cmd, "PCHANDLER");
}
