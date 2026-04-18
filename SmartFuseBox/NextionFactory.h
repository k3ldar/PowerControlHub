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

#if defined(NEXTION_DISPLAY_DEVICE)

#include <NextionControl.h>
#include <vector>
#include <memory>

#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
#include <SoftwareSerial.h>
#endif

#include "ConfigManager.h"
#include "WarningManager.h"
#include "WarningType.h"
#include "NavigationController.h"
#include <SerialCommandManager.h>
#include "PageAbout.h"
#include "PageBuoys.h"
#include "PageCardinalMarkers.h"
#include "PageEnvironment.h"
#include "PageFlags.h"
#include "PageHome.h"
#include "PageMoonPhase.h"
#include "PageRelay.h"
#include "PageRelaySettings.h"
#include "PageSettings.h"
#include "PageSoundEmergency.h"
#include "PageSoundFog.h"
#include "PageSoundManeuvering.h"
#include "PageSoundOther.h"
#include "PageSoundOvertaking.h"
#include "PageSoundSignals.h"
#include "PageSplash.h"
#include "PageSystem.h"
#include "PageVhfChannels.h"
#include "PageVhfDistress.h"
#include "PageVhfRadio.h"
#include "PageWarning.h"


class NextionFactory
{
public:
	// Create and return an owning pointer to a configured NextionControl.
	// The created page instances are kept in internal static storage so their
	// lifetime matches the returned controller. Returns nullptr if Nextion is
	// disabled in config or serial initialisation fails.
	// locationType controls which pages are included in next/previous navigation.
	static NextionControl* Create(
		WarningManager* warningManager,
		SerialCommandManager* commandMgrComputer,
		SoundController* soundController,
		RelayController* relayController,
		LocationType locationType);

private:
	// Initialises and returns the serial port for the Nextion display.
	// The port is initialised once and reused on subsequent calls.
	// Board-conditional:
	//   ESP32            – HardwareSerial UART2 with runtime pin assignment.
	//   ARDUINO_UNO_R4
	//   ARDUINO_R4_MINIMA – Fixed Serial1 peripheral (pins set by hardware).
	//   All boards       – SoftwareSerial when isHardwareSerial == false.
	static Stream* initSerial(const NextionConfig& cfg);
};

inline Stream* NextionFactory::initSerial(const NextionConfig& cfg)
{
	static Stream* s_serial = nullptr;

	if (s_serial != nullptr)
		return s_serial;

	if (cfg.isHardwareSerial)
	{
#if defined(ESP32)
		// UART0 is reserved for USB/debug and must never be used for Nextion.
		// Valid values are 1 or 2; anything else (including 0/unset) defaults to UART2.
		const uint8_t uart = (cfg.uartNum == 1 || cfg.uartNum == 2) ? cfg.uartNum : 2;
		static HardwareSerial s_hwSerial(uart);
		s_hwSerial.begin(cfg.baudRate, SERIAL_8N1, cfg.rxPin, cfg.txPin);
		s_serial = &s_hwSerial;
#elif defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
		// Hardware serial pins are fixed on the R4; Serial1 maps to the header.
		// rxPin/txPin in config are ignored for hardware serial on this board.
		Serial1.begin(cfg.baudRate);
		s_serial = &Serial1;
#endif
	}
#if defined(ARDUINO_UNO_R4) || defined(ARDUINO_R4_MINIMA)
	else
	{
		// SoftwareSerial: available on all target boards.
		// The static local is constructed with the config pins on the first call
		// and reused on all subsequent calls.
		static SoftwareSerial s_swSerial(cfg.rxPin, cfg.txPin);
		s_swSerial.begin(cfg.baudRate);
		s_serial = &s_swSerial;
	}
#endif

	return s_serial;
}

inline NextionControl* NextionFactory::Create(
	WarningManager* warningManager,
	SerialCommandManager* commandMgrComputer,
	SoundController* soundController,
	RelayController* relayController,
	LocationType locationType)
{
	// Singleton: factory owns the instance for the program lifetime.
	static std::unique_ptr<NextionControl> s_instance;

	if (s_instance != nullptr)
		return s_instance.get();

	// Storage for owned page objects — static so pages outlive the controller.
	static std::vector<std::unique_ptr<BaseDisplayPage>> s_pages;
	static std::vector<BaseDisplayPage*> s_pagePtrs;
	static std::unique_ptr<NavigationController> s_navController;

	Config* config = ConfigManager::getConfigPtr();

	if (config == nullptr || !config->nextion.enabled)
		return nullptr;

#if defined(ESP32)
	// SoftwareSerial is not supported on ESP32. Raise a warning and bail out
	// before attempting any serial initialisation.
	if (!config->nextion.isHardwareSerial)
	{
		if (warningManager != nullptr)
			warningManager->raiseWarning(WarningType::NextionInvalidConfig);

		return nullptr;
	}
#endif

	Stream* serialPort = initSerial(config->nextion);

	// initSerial returns nullptr if the board/config combination is unsupported.
	if (serialPort == nullptr)
	{
		if (warningManager != nullptr)
			warningManager->raiseWarning(WarningType::NextionInvalidConfig);

		return nullptr;
	}

	if (s_pages.empty())
	{
		// Order must match the page numbering expected by the Nextion UI!
		s_pages.emplace_back(std::make_unique<PageSplash>(serialPort));
		s_pages.emplace_back(std::make_unique<PageHome>(serialPort, warningManager, commandMgrComputer, relayController));
		s_pages.emplace_back(std::make_unique<PageWarning>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageRelay>(serialPort, warningManager, commandMgrComputer, relayController));
		s_pages.emplace_back(std::make_unique<PageSoundSignals>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSoundOvertaking>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSoundFog>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSoundManeuvering>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSoundEmergency>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSoundOther>(serialPort, warningManager, commandMgrComputer, soundController));
		s_pages.emplace_back(std::make_unique<PageSystem>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageFlags>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageCardinalMarkers>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageBuoys>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageMoonPhase>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageVhfRadio>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageVhfDistress>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageVhfChannels>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageRelaySettings>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageSettings>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageEnvironment>(serialPort, warningManager, commandMgrComputer));
		s_pages.emplace_back(std::make_unique<PageAbout>(serialPort, warningManager, commandMgrComputer));

		// Build raw pointer array for NextionControl (non-owning).
		s_pagePtrs.reserve(s_pages.size());
		for (auto& p : s_pages)
			s_pagePtrs.push_back(p.get());
	}

	s_instance = std::unique_ptr<NextionControl>(
		new NextionControl(serialPort, s_pagePtrs.data(), s_pagePtrs.size()));

	// Create the navigation controller and inject it into all BasePage instances.
	// PageSplash inherits BaseDisplayPage directly and is intentionally skipped.
	s_navController = std::make_unique<NavigationController>(s_instance.get(), locationType);

    // Inject navigation delegate into all pages that inherit from BasePage.
	// PageSplash (index 0) intentionally derives from BaseDisplayPage and is
	// skipped. We avoid dynamic_cast because RTTI is disabled in some build
	// configurations (-fno-rtti). Use static_cast for pages we created and
	// skip the splash page which does not implement BasePage.
	for (size_t i = 1; i < s_pagePtrs.size(); ++i)
	{
		BasePage* bp = static_cast<BasePage*>(s_pagePtrs[i]);
		if (bp)
			bp->setNavigationDelegate(s_navController.get());
	}

	return s_instance.get();
}

#endif