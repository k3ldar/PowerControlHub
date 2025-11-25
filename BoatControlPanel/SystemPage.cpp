
#include "SystemPage.h"
#include "SharedFunctions.h"
#include "SystemCpuMonitor.h"

constexpr char ControlFuseBoxCpu[] = "t5";
constexpr char ControlFuseBoxMemory[] = "t6";
constexpr char ControlPanelCpu[] = "t7";
constexpr char ControlPanelMemory[] = "t8";

constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;


constexpr unsigned long RefreshSystemIntervalMs = 1000;

constexpr char BytesSuffix[] = " bytes";
constexpr char MemoryFormat[] = "%d bytes";
constexpr char CpuFormat[] = "%d%%";

SystemPage::SystemPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{

}

void SystemPage::begin()
{

}

void SystemPage::onEnterPage()
{

    // Request relay states to update button states
    getCommandMgrLink()->sendCommand(SystemFreeMemory, "");
    getCommandMgrLink()->sendCommand(SystemCpuUsage, "");
    _lastRefreshTime = millis();

    updateAllDisplayItems();
}

void SystemPage::refresh(unsigned long now)
{
    updateAllDisplayItems();

    if (now - _lastRefreshTime >= RefreshSystemIntervalMs)
    {
        SerialCommandManager* linkMgr = getCommandMgrLink();
        SerialCommandManager* compMgr = getCommandMgrComputer();

        if (compMgr)
        {
            compMgr->sendDebug(F("Sending F2/F3"), F("SystemPage"));
        }

        _lastRefreshTime = now;

        if (linkMgr)
        {
            linkMgr->sendCommand(SystemFreeMemory, "");
            linkMgr->sendCommand(SystemCpuUsage, "");
        }

        updateControlPanelCpu();
        updateControlPanelMemory();
    }
}

void SystemPage::updateAllDisplayItems()
{
    updateControlPanelCpu();
    updateControlPanelMemory();
    updateFuseBoxCpu();
    updateFuseBoxMemory();
}

// Handle touch events for buttons
void SystemPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    switch (compId)
    {
    case ButtonPrevious:
        setPage(PageSoundSignals);
        break;

    case ButtonNext:
        setPage(PageAbout);
        return;

    default:
        return;
    }
}

void SystemPage::handleExternalUpdate(uint8_t updateType, const void* data)
{
    getCommandMgrComputer()->sendDebug("SystemPage::handleExternalUpdate type=" + String(updateType), F("SystemPage"));

    // Call base class first to handle heartbeat ACKs
    BaseBoatPage::handleExternalUpdate(updateType, data);

    if (updateType == static_cast<uint8_t>(PageUpdateType::CpuUsage) && data != nullptr)
    {
        const UInt8Update* update = static_cast<const UInt8Update*>(data);
		setFuseBoxCpu(update->value);
    }
    else if (updateType == static_cast<uint8_t>(PageUpdateType::MemoryUsage) && data != nullptr)
    {
        const UInt16Update* update = static_cast<const UInt16Update*>(data);
        setFuseBoxMemory(update->value);
    }
}

// --- Public setters ---

void SystemPage::setFuseBoxCpu(uint8_t cpu)
{
	if (_lastFuseBoxCpu != cpu)
    {
        _lastFuseBoxCpu = cpu;
        updateFuseBoxCpu();
    }
}

void SystemPage::setFuseBoxMemory(uint16_t memory)
{
	if (_lastFuseBoxMemory != memory)
    {
        _lastFuseBoxMemory = memory;
        updateFuseBoxMemory();
    }
}


// --- Private update methods ---
void SystemPage::updateControlPanelCpu()
{
	uint8_t cpu = SystemCpuMonitor::getCpuUsage();

    if (cpu != _lastControlPanelCpu)
    {
        _lastControlPanelCpu = cpu;
        sendText(ControlPanelCpu, String(cpu) + PercentSuffix);
    }
}

void SystemPage::updateControlPanelMemory()
{
	uint16_t memory = SharedFunctions::freeMemory();

	if (memory != _lastControlPanelMemory)
    {
        _lastControlPanelMemory = memory;
        sendText(ControlPanelMemory, String(memory) + BytesSuffix);
    }
}

void SystemPage::updateFuseBoxCpu()
{
    if (_lastFuseBoxCpu == MaxUint8Value)
    {
        sendText(ControlFuseBoxCpu, NoValueText);
        return;
    }

    char buffer[12];
    snprintf(buffer, sizeof(buffer), CpuFormat, _lastFuseBoxCpu);
    sendText(ControlFuseBoxCpu, buffer);
}

void SystemPage::updateFuseBoxMemory()
{
    if (_lastFuseBoxMemory == MaxUint16Value)
    {
        sendText(ControlFuseBoxMemory, NoValueText);
        return;
    }

    char buffer[16];
    snprintf(buffer, sizeof(buffer), MemoryFormat, _lastFuseBoxMemory);
    sendText(ControlFuseBoxMemory, buffer);
}
