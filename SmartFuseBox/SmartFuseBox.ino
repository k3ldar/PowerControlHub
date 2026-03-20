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
#include <Arduino.h>
#include <SerialCommandManager.h>

#include "Local.h"
#include "SmartFuseBoxApp.h"
#include "SystemFunctions.h"

#if defined(SUB_CONTROLLER_WEATHER_STATION)
#include "WaterSensorHandler.h"
#include "LightSensorHandler.h"
#endif

#include "Dht11SensorHandler.h"
#include "SystemSensorHandler.h"

#include "RemoteSensor.h"

// BaseSensor.h is required here because the localSensors[] array below is typed
// as BaseSensorHandler*. When MQTT_SUPPORT is active, RemoteSensor inherits from
// a different base that is not pulled in by the sensor-specific headers above.
#if defined(MQTT_SUPPORT)
#include "BaseSensor.h"
#endif

// Hardware UART selection. Change these two defines if your board routes its
// serial ports differently (e.g. swap Serial1 for Serial2 on a Mega-class board).
// COMPUTER_SERIAL – connected to the host PC / debug monitor.
// LINK_SERIAL     – connected to a secondary controller or companion board.
#define COMPUTER_SERIAL Serial
#define LINK_SERIAL Serial1

// forward declares
void onComputerCommandReceived(SerialCommandManager* mgr);
void onLinkCommandReceived(SerialCommandManager* mgr);


// Consumer note:
// - `commandMgrComputer` and `commandMgrLink` are local to your .ino so you can
//   select the hardware Serial instances (Serial, Serial1, etc.) and baud rates
//   appropriate for your board. Keep the callbacks `onComputerCommandReceived` and
//   `onLinkCommandReceived` in this file so they can access these serial managers.
// - Construct `SmartFuseBoxApp` with pointers to these serial managers and your
//   relay pin mapping. Then construct any board-specific sensors using the
//   accessors from `app` (e.g. `app.messageBus()`, `app.broadcastManager()`,
//   `app.sensorCommandHandler()`). Finally call `app.setup(...)` with your
//   sensor arrays from `setup()` and call `app.loop()` from `loop()`.

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', ';', '=', 500, 64);
SerialCommandManager commandMgrLink(&LINK_SERIAL, onLinkCommandReceived, '\n', ':', ';', '=', 500, 64);

SmartFuseBoxApp app(&commandMgrComputer, &commandMgrLink, Relays, ConfigRelayCount);

// Project-specific sensors
#if defined(SUB_CONTROLLER_WEATHER_STATION)
WaterSensorHandler waterSensorHandler(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), WaterSensorPin, WaterSensorActivePin);
Dht11SensorHandler dht11SensorHandler(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.warningManager(), Dht11SensorPin);
LightSensorHandler lightSensorHandler(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.relayController(),
	LightSensorPin, LightSensorIsDigital);
#endif

#if defined(SUB_CONTROLLER_BASEMENT_SENSOR)
Dht11SensorHandler basementStoreRoom(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.warningManager(), Dht11StoreRoomSensorPin, "StoreRoom");
Dht11SensorHandler basementMakerRoom(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.warningManager(), Dht11MakerRoomSensorPin, "MakerRoom");
Dht11SensorHandler basementPassageway(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.warningManager(), Dht11PassagewaySensorPin, "PassageWay");
Dht11SensorHandler basementWashRoom(app.messageBus(), app.broadcastManager(),
	app.sensorCommandHandler(), app.warningManager(), Dht11WashRoomSensorPin, "WashRoom");
#endif

SystemSensorHandler systemSensorHandler(app.messageBus(),
	app.wifiController(),
	app.bluetoothController(),
	app.warningManager());

// Project specific remote sensors
#if defined(MQTT_SUPPORT)
MqttSensorChannel gpsMqttChannels[] = {
	{ "Latitude", "latitude", "latitude", nullptr, "°", false},
	{ "Longitude", "longitude", "longitude", nullptr, "°", false}
};
RemoteSensor gpsLatLonSensor(SensorIdList::GpsSensor, "Gps", SensorGpsLatLong, "Gps", gpsMqttChannels, 2);
#else
RemoteSensor gpsLatLonSensor(SensorIdList::GpsSensor, "Gps", SensorGpsLatLong, 2);
#endif

RemoteSensor* remoteSensors[] = {
	&gpsLatLonSensor
};
constexpr uint8_t remoteSensorCount = sizeof(remoteSensors) / sizeof(remoteSensors[0]);


// All sensors polled by the main loop — both board-local and remote.
// Note: gpsLatLonSensor appears in remoteSensors[] (for the remote-sync pipeline)
// AND here (so it is also ticked by the local poll loop for status reporting).
BaseSensorHandler* localSensors[] = {
#if defined(SUB_CONTROLLER_WEATHER_STATION)
	&waterSensorHandler, &dht11SensorHandler, &lightSensorHandler, &systemSensorHandler, &gpsLatLonSensor
#elif defined(SUB_CONTROLLER_BASEMENT_SENSOR)
	&basementStoreRoom, &basementMakerRoom, &basementPassageway, &basementWashRoom, & systemSensorHandler, &gpsLatLonSensor
#endif
};
constexpr uint8_t sensorHandlerCount = sizeof(localSensors) / sizeof(localSensors[0]);

void setup()
{
	// Serial initialization is performed first to ensure that any logging or error messages
	// from DateTimeManager or ConfigManager during initialization are properly output.
	SystemFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);
	SystemFunctions::initializeSerial(LINK_SERIAL, 19200, true);

	// Wire late-binding sensor dependencies
#if defined(SD_CARD_SUPPORT)
	systemSensorHandler.setSdCardLogger(app.sdCardLogger());
#endif

	// configure app
	app.setup(localSensors, sensorHandlerCount, remoteSensors, remoteSensorCount);
}

void loop()
{
	app.loop();
}

// Called when a command arrives on COMPUTER_SERIAL that no registered handler
// claimed. Reports the unrecognised message back to the sender as an error and
// resets the serial buffer to discard any partial frame that may follow.
void onComputerCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATCMD"));
	SystemFunctions::resetSerial(COMPUTER_SERIAL);
}

// Called when a command arrives on LINK_SERIAL that no registered handler
// claimed. Forwards the error notification to the computer-side monitor
// (not the link serial) so the host can log it, then resets the link buffer.
void onLinkCommandReceived(SerialCommandManager* mgr)
{
	commandMgrComputer.sendError(mgr->getRawMessage(), F("STATLNK"));
	SystemFunctions::resetSerial(LINK_SERIAL);
}

