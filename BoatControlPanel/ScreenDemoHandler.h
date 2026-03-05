#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>
#include <BaseCommandHandler.h>
#include <NextionControl.h>
#include "NextionIds.h"

constexpr char ScreenDemoCommand[] = "SCRNDMO";
constexpr uint16_t DefaultDemoIntervalMs = 6000;

class ScreenDemoHandler : public BaseCommandHandler
{
private:
    NextionControl* _nextionControl;
    uint8_t _totalPages;

public:
    ScreenDemoHandler(NextionControl* nextionControl, uint8_t pageCount)
        : _nextionControl(nextionControl),
          _totalPages(pageCount)
    {
    }

    bool supportsCommand(const char* command) const override
    {
        return strcmp(command, ScreenDemoCommand) == 0;
    }

    bool handleCommand(SerialCommandManager* sender, const char* command, const StringKeyValue params[], uint8_t paramCount) override
    {
        (void)command;

        uint16_t intervalMs = DefaultDemoIntervalMs;
        for (uint8_t i = 0; i < paramCount; i++)
        {
            if (strcmp(params[i].key, "s") == 0)
            {
                unsigned long secs = strtoul(params[i].value, nullptr, 10);
                if (secs > 0 && secs <= 60)
                {
                    intervalMs = static_cast<uint16_t>(secs * 1000UL);
                }
            }
        }

        for (uint8_t pageId = PageHome; pageId < _totalPages; pageId++)
        {
            char pageCmd[12];
            snprintf_P(pageCmd, sizeof(pageCmd), PSTR("page %u"), pageId);
            _nextionControl->sendCommand(String(pageCmd));
            delay(intervalMs);
        }

        char pageCmd[12];
        snprintf_P(pageCmd, sizeof(pageCmd), PSTR("page %u"), PageHome);
        _nextionControl->sendCommand(String(pageCmd));

        sendAckOk(sender, ScreenDemoCommand);
        return true;
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        count = 0;
        return nullptr;
    }
};


