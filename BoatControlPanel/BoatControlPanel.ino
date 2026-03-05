#include "ScreenDemoHandler.h"
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
#include "SunCalculator.h"

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
#include "MoonPhasePage.h"
#include "VhfRadioPage.h"
#include "VhfChannelsPage.h"
#include "VhfDistressPage.h"
#include "SettingsPage.h"
#include "RelaySettingsPage.h"
#include "EnvironmentPage.h"
#include "AboutPage.h"

#include "Config.h"
#include "ConfigManager.h"
#include "WarningManager.h"
#include "ToneManager.h"
#include "RgbLedFade.h"

#include "GpsSensorHandler.h"

// include
#include "ScreenDemoHandler.h"

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

// sound
ToneManager toneManager(PinTone);

// led indicators
RgbLedFade systemLedStatus(PinRgbRed, PinRgbGreen, PinRgbBlue);

// Serial managers
SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', ';', '=', 512, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', ';', '=', 1024, 64);

// Broadcast manager for coordinated messaging
BroadcastManager broadcastManager(&commandMgrComputer, &commandMgrLink);

// Warning manager with heartbeat monitoring
WarningManager warningManager(&commandMgrLink, HeartbeatIntervalMs, HeartbeatTimeoutMs, &systemLedStatus, &toneManager);

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
MoonPhasePage moonPhasePage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
VhfRadioPage radioPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
VhfDistressPage radioPageDistress(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
VhfChannelsPage radioPageChannels(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
SettingsPage settingsPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
RelaySettingsPage relaySettingsPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
EnvironmentPage environmentPage(&NEXTION_SERIAL, &warningManager, &commandMgrLink, &commandMgrComputer);
AboutPage aboutPage(&NEXTION_SERIAL);

BaseDisplayPage* displayPages[] = { &splashPage, &homePage, &warningPage, &relayPage, &soundSignalsPage, 
    &soundOvertakingPage, &soundFogPage, &soundManeuveringPage, &soundEmergencyPage, &soundOtherPage,
    &systemPage, &flagsPage, & cardinalMarkersPage, &buoysPage, &moonPhasePage, &radioPage, 
    &radioPageDistress, &radioPageChannels, &relaySettingsPage, &settingsPage, &environmentPage,
    &aboutPage };
NextionControl nextion(&NEXTION_SERIAL, displayPages, sizeof(displayPages) / sizeof(displayPages[0]));

// global instance, after nextion declaration
ScreenDemoHandler screenDemoHandler(&nextion, sizeof(displayPages) / sizeof(displayPages[0]));

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

BaseSensorHandler* sensorHandlers[] = {
    &gpsSensor
};
uint8_t sensorHandlerCount = sizeof(sensorHandlers) / sizeof(sensorHandlers[0]);
SensorManager sensorManager(sensorHandlers, sensorHandlerCount);

// Timers
unsigned long lastUpdate = 0;

void setup()
{
    systemLedStatus.begin();
    systemLedStatus.setFadeSpeed(0.008f);
    systemLedStatus.setMaxBrightness(60);
    systemLedStatus.setMinBrightness(10);
    systemLedStatus.setWarning(false);
    systemLedStatus.setDayTime(true); // Start in day mode
    systemLedStatus.setColor(120, 0, 180);
    systemLedStatus.update(0);

    DateTimeManager::begin();

    ISerialCommandHandler* linkHandlers[] = { &interceptDebugHandler, &ackHandler, &sensorCommandHandler,
        &warningCommandHandler, &systemCommandHandler, &configHandler };
    size_t linkHandlerCount = sizeof(linkHandlers) / sizeof(linkHandlers[0]);
    commandMgrLink.registerHandlers(linkHandlers, linkHandlerCount);

    ISerialCommandHandler* computerHandlers[] = { &configHandler, &ackHandler, &sensorCommandHandler, 
        &warningCommandHandler, &systemCommandHandler, &screenDemoHandler };
    size_t computerHandlerCount = sizeof(computerHandlers) / sizeof(computerHandlers[0]);
    commandMgrComputer.registerHandlers(computerHandlers, computerHandlerCount);

    SystemFunctions::initializeSerial(COMPUTER_SERIAL, BaudRateComputer, true);

#if defined(ARDUINO_MEGA2560)
    SystemFunctions::initializeSerial(NEXTION_SERIAL, BaudRateDefault, false);
    SystemFunctions::initializeSerial(LINK_SERIAL, BaudRateDefault, false);
	SystemFunctions::initializeSerial(GPS_SERIAL, BaudRateGps, false);
#elif defined(ARDUINO_R4_MINIMA)
	NEXTION_SERIAL.begin(BaudRateDefault);
	LINK_SERIAL.begin(BaudRateDefault);
	GPS_SERIAL.begin(BaudRateGps);
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
    DateTimeManager::setTimezoneOffset(config->timezoneOffset);
    homePage.configSet(config);
    warningPage.configSet(config);
	relayPage.configSet(config);
    radioPage.configSet(config);
	radioPageDistress.configSet(config);
    settingsPage.configSet(config);
	relaySettingsPage.configSet(config);
	environmentPage.configSet(config);
	systemLedStatus.configSet(config);
	toneManager.configSet(&config->soundConfig);

	systemLedStatus.setDayTime(true);

	nextion.begin();

	sensorManager.setup();

	char buffer[20];
	snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), config->hornRelayIndex);
	broadcastManager.sendCommand(ConfigSoundRelayId, buffer);
	snprintf_P(buffer, sizeof(buffer), PSTR("v=%u"), static_cast<uint8_t>(config->vesselType));
	broadcastManager.sendCommand(ConfigBoatType, buffer);

	if (DateTimeManager::isTimeSet())
	{
		snprintf_P(buffer, sizeof(buffer), PSTR("v=%lu"), DateTimeManager::getCurrentTime());
		broadcastManager.sendCommand(SystemSetDateTime, buffer);
	}

	broadcastManager.sendCommand(SystemInitialized, "");

	nextion.sendCommand(PageOne);
	toneManager.play(ToneType::Good);
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
    systemLedStatus.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::startTask();
    toneManager.update(now);
    SystemCpuMonitor::endTask();

    SystemCpuMonitor::update();
}

void onLinkCommandReceived(SerialCommandManager* mgr)
{
    char cmd[64];
	snprintf_P(cmd, sizeof(cmd), PSTR("%s"), mgr->getCommand());
    commandMgrComputer.sendError(cmd, F("LINKHANDLER"));

	// Reset serial to clear any residual data
	SystemFunctions::resetSerial(LINK_SERIAL);
}

void onComputerCommandReceived(SerialCommandManager* mgr)
{
    char cmd[64];
	snprintf_P(cmd, sizeof(cmd), PSTR("%s"), mgr->getCommand());

    commandMgrComputer.sendError(cmd, F("PCHANDLER"));

    // Reset serial to clear any residual data
    SystemFunctions::resetSerial(COMPUTER_SERIAL);
}
