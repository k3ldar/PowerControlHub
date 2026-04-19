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

#include "PageSystem.h"

#if defined(NEXTION_DISPLAY_DEVICE)

#include "SystemFunctions.h"
#include "SystemCpuMonitor.h"

constexpr char ControlFuseBoxCpu[] = "t5";
constexpr char ControlFuseBoxMemory[] = "t6";
constexpr char ControlPanelCpu[] = "t7";
constexpr char ControlPanelMemory[] = "t8";

constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;


constexpr unsigned long RefreshSystemIntervalMs = 500;

constexpr char BytesSuffix[] = " bytes";
const char MemoryFormat[] PROGMEM = "%d bytes";
const char CpuFormat[] PROGMEM = "%d%%";

PageSystem::PageSystem(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrComputer,
    MessageBus* messageBus)
    : BasePage(serialPort, warningMgr, commandMgrComputer, messageBus)
{
    if (messageBus)
    {
        messageBus->subscribe<CpuUsageUpdated>([this](uint8_t usage) {
            if (isActive()) setFuseBoxCpu(usage);
        });
        messageBus->subscribe<MemoryUsageUpdated>([this](uint16_t freeBytes) {
            if (isActive()) setFuseBoxMemory(freeBytes);
        });
    }
}

void PageSystem::begin()
{

}

void PageSystem::onEnterPage()
{
    _lastRefreshTime = millis();

    updateAllDisplayItems();
}

void PageSystem::refresh(unsigned long now)
{
    updateAllDisplayItems();

    if (now - _lastRefreshTime >= RefreshSystemIntervalMs)
    {
        SerialCommandManager* compMgr = getCommandMgrComputer();

        if (compMgr)
        {
            compMgr->sendDebug(F("Sending F2/F3"), F("SystemPage"));
        }

        _lastRefreshTime = now;

        setFuseBoxCpu(SystemCpuMonitor::getCpuUsage());
        setFuseBoxMemory(SystemFunctions::freeMemory());

        updateControlPanelCpu();
        updateControlPanelMemory();
    }
}

void PageSystem::updateAllDisplayItems()
{
    updateControlPanelCpu();
    updateControlPanelMemory();
    updateFuseBoxCpu();
    updateFuseBoxMemory();
}

// Handle touch events for buttons
void PageSystem::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;
    
    switch (compId)
    {
    case ButtonPrevious:
        navigatePrevious();
        break;

    case ButtonNext:
        navigateNext();
        return;

    default:
        return;
    }
}

// --- Public setters ---

void PageSystem::setFuseBoxCpu(uint8_t cpu)
{
	if (_lastFuseBoxCpu != cpu)
    {
        _lastFuseBoxCpu = cpu;
        updateFuseBoxCpu();
    }
}

void PageSystem::setFuseBoxMemory(uint16_t memory)
{
	if (_lastFuseBoxMemory != memory)
    {
        _lastFuseBoxMemory = memory;
        updateFuseBoxMemory();
    }
}


// --- Private update methods ---
void PageSystem::updateControlPanelCpu()
{
	uint8_t cpu = SystemCpuMonitor::getCpuUsage();

	if (cpu != _lastControlPanelCpu)
	{
		_lastControlPanelCpu = cpu;
		char value[10];
		snprintf_P(value, sizeof(value), CpuFormat, cpu);
		sendText(ControlPanelCpu, value);
	}
}

void PageSystem::updateControlPanelMemory()
{
	uint16_t memory = SystemFunctions::freeMemory();

	if (memory != _lastControlPanelMemory)
	{
		_lastControlPanelMemory = memory;
		char value[15];
		snprintf_P(value, sizeof(value), MemoryFormat, memory);
		sendText(ControlPanelMemory, value);
	}
}

void PageSystem::updateFuseBoxCpu()
{
    if (_lastFuseBoxCpu == MaxUint8Value)
    {
        sendText(ControlFuseBoxCpu, NoValueText);
        return;
    }

    char buffer[12];
    snprintf_P(buffer, sizeof(buffer), CpuFormat, _lastFuseBoxCpu);
    sendText(ControlFuseBoxCpu, buffer);
}

void PageSystem::updateFuseBoxMemory()
{
    if (_lastFuseBoxMemory == MaxUint16Value)
    {
        sendText(ControlFuseBoxMemory, NoValueText);
        return;
    }

    char buffer[16];
    snprintf_P(buffer, sizeof(buffer), MemoryFormat, _lastFuseBoxMemory);
    sendText(ControlFuseBoxMemory, buffer);
}

#endif // NEXTION_DISPLAY_DEVICE
