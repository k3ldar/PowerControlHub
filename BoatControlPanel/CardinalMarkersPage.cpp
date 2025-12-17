// 
// 
// 

#include "CardinalMarkersPage.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;

CardinalMarkersPage::CardinalMarkersPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer)
{
}

void CardinalMarkersPage::begin()
{
}

void CardinalMarkersPage::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void CardinalMarkersPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case ButtonNext:
        setPage(PageBuoys);
        return;

    case ButtonPrevious:
        setPage(PageFlags);
        return;
    }
}
