#pragma once

#include <NextionControl.h>
#include "NextionIds.h"

class AboutPage : public BaseDisplayPage
{
public:
    AboutPage(Stream* serialPort);

protected:

    uint8_t getPageId() const override { return PageAbout; }
    void begin() override;
    void refresh(unsigned long now) override;

    //optional overrides
    void handleTouch(uint8_t compId, uint8_t eventType) override;
};