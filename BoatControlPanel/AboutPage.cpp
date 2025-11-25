
#include "AboutPage.h"

// Nextion Names/Ids on current Page
constexpr uint8_t BtnNext = 3; // b3
constexpr uint8_t BtnBack = 2; // b4

constexpr unsigned long RefreshIntervalMs = 10000;

AboutPage::AboutPage(Stream* serialPort)
    : BaseDisplayPage(serialPort)
{
}

void AboutPage::begin()
{

}

void AboutPage::refresh(unsigned long now)
{
    (void)now;
}

// Handle touch events for buttons
void AboutPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case BtnNext:
		setPage(PageHome);
        break;

    case BtnBack:
        setPage(PageSystem);
        break;

    }
}
