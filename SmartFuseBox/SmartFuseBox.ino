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

#include "WaterSensorHandler.h"
#include "Dht11SensorHandler.h"
#include "LightSensorHandler.h"
#include "SystemSensorHandler.h"
#include "GpsSensorHandler.h"

 // Hardware UART selection. Change this define if your board routes its serial
// port differently.
// COMPUTER_SERIAL – connected to the host PC / debug monitor.
// GPS_SERIAL      – connected to the GPS module.
#define COMPUTER_SERIAL Serial
#define GPS_SERIAL Serial2

// forward declares
void onComputerCommandReceived(SerialCommandManager* mgr);


// Consumer note:
// - `commandMgrComputer` is local to your .ino so you can select the hardware
//   Serial instance and baud rate appropriate for your board. Keep the callback
//   `onComputerCommandReceived` in this file so it can access the serial manager.
// - Construct `SmartFuseBoxApp` with a pointer to the serial manager. Then
//   configure board-specific resources (e.g. GPS serial) via app accessors before
//   calling `app.setup()`. Call `app.loop()` from `loop()`.

SerialCommandManager commandMgrComputer(&COMPUTER_SERIAL, onComputerCommandReceived, '\n', ':', ';', '=', 500, 64);

SmartFuseBoxApp app(&commandMgrComputer);

void setup()
{
	// Serial initialization is performed first to ensure that any logging or error messages
	// from DateTimeManager or ConfigManager during initialization are properly output.
	SystemFunctions::initializeSerial(COMPUTER_SERIAL, 115200, true);
	SystemFunctions::initializeSerial(GPS_SERIAL, GpsBaudRate, false);

	app.setGpsSerial(&GPS_SERIAL);

	// configure app
	app.setup(nullptr, 0);
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

#if defined(NEXTION_DISPLAY_DEVICE) && defined(NEXTION_DEBUG)
nextion.setDebugCallback([](const String& msg)
	{
		commandMgrComputer.sendCommand(msg.c_str(), "NEXTION");
	});
#endif

