#include "VhfChannelsPage.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonPrevious = 2;

VhfChannelsPage::VhfChannelsPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void VhfChannelsPage::onEnterPage()
{
    // not used just now
}

void VhfChannelsPage::begin()
{
    // not used just now
}

void VhfChannelsPage::refresh(unsigned long now)
{
    //nothing to refresh currently
}

// Handle touch events for buttons
void VhfChannelsPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonPrevious:
        setPage(PageVhfRadio);
        return;
    }
}
