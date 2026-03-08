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

#include <Arduino.h>
#include <SerialCommandManager.h>
#include <BaseCommandHandler.h>
#include <NextionControl.h>
#include "NextionIds.h"
#include "Local.h"

#if defined(SCREEN_DEMO_SUPPORT)

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

#endif