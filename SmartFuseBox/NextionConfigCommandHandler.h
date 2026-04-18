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

#include "BaseConfigCommandHandler.h"
#include "ConfigController.h"
#include "SystemDefinitions.h"

/**
 * @brief Command handler for Nextion display configuration (N-series commands).
 *
 * Each setting is exposed as its own command so no single message ever exceeds
 * the 5-parameter limit imposed by SerialCommandManager.
 *
 * Command reference:
 *   N0          — Get all Nextion config (broadcasts N1–N6 with current values)
 *   N1:v=<0|1>  — Set enabled state
 *   N2:v=<0|1>  — Set serial mode (1=hardware, 0=software)
 *   N3:v=<pin>  — Set RX pin (255/PinDisabled to clear)
 *   N4:v=<pin>  — Set TX pin (255/PinDisabled to clear)
 *   N5:v=<baud> — Set baud rate
 *   N6:v=<1|2>  — Set UART number (1 or 2 only; UART0 reserved for USB/debug)
 */
class NextionConfigCommandHandler : public virtual BaseCommandHandler, public BaseConfigCommandHandler
{
private:
    ConfigController* _configController;

    void broadcastNextionConfig(SerialCommandManager* sender) const
    {
        Config* config = ConfigManager::getConfigPtr();
        if (config == nullptr)
            return;

        char buf[24];

        snprintf(buf, sizeof(buf), "v=%u", config->nextion.enabled ? 1u : 0u);
        sender->sendCommand(NextionEnabled, buf);

        snprintf(buf, sizeof(buf), "v=%u", config->nextion.isHardwareSerial ? 1u : 0u);
        sender->sendCommand(NextionHardwareSerial, buf);

        snprintf(buf, sizeof(buf), "v=%u", config->nextion.rxPin);
        sender->sendCommand(NextionRxPin, buf);

        snprintf(buf, sizeof(buf), "v=%u", config->nextion.txPin);
        sender->sendCommand(NextionTxPin, buf);

        snprintf(buf, sizeof(buf), "v=%lu", config->nextion.baudRate);
        sender->sendCommand(NextionBaudRate, buf);

        snprintf(buf, sizeof(buf), "v=%u", config->nextion.uartNum);
        sender->sendCommand(NextionUartNum, buf);
    }

public:
    explicit NextionConfigCommandHandler(ConfigController* configController)
        : _configController(configController)
    {
    }

    const char* const* supportedCommands(size_t& count) const override
    {
        static const char* cmds[] = {
            NextionGetConfig,
            NextionEnabled,
            NextionHardwareSerial,
            NextionRxPin,
            NextionTxPin,
            NextionBaudRate,
            NextionUartNum
        };
        count = sizeof(cmds) / sizeof(cmds[0]);
        return cmds;
    }

    bool handleCommand(SerialCommandManager* sender, const char* command,
                       const StringKeyValue params[], uint8_t paramCount) override
    {
        if (SystemFunctions::commandMatches(command, NextionGetConfig))
        {
            broadcastNextionConfig(sender);
            sendAckOk(sender, command);
            return true;
        }

        if (_configController == nullptr)
        {
            sendAckErr(sender, command, F("Config not available"));
            return true;
        }

        ConfigResult result = ConfigResult::InvalidCommand;

        if (SystemFunctions::commandMatches(command, NextionEnabled))
        {
            if (paramCount >= 1)
            {
                bool enabled;
                if (!getParamValueBool(params, paramCount, "v", enabled))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionEnabled(enabled);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }
        else if (SystemFunctions::commandMatches(command, NextionHardwareSerial))
        {
            if (paramCount >= 1)
            {
                bool hwSerial;
                if (!getParamValueBool(params, paramCount, "v", hwSerial))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionHardwareSerial(hwSerial);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }
        else if (SystemFunctions::commandMatches(command, NextionRxPin))
        {
            if (paramCount >= 1)
            {
                uint8_t pin;
                if (!getParamValueU8t(params, paramCount, "v", pin))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionRxPin(pin);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }
        else if (SystemFunctions::commandMatches(command, NextionTxPin))
        {
            if (paramCount >= 1)
            {
                uint8_t pin;
                if (!getParamValueU8t(params, paramCount, "v", pin))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionTxPin(pin);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }
        else if (SystemFunctions::commandMatches(command, NextionBaudRate))
        {
            if (paramCount >= 1)
            {
                uint32_t baud;
                if (!getParamValueU32t(params, paramCount, "v", baud))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionBaudRate(baud);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }
        else if (SystemFunctions::commandMatches(command, NextionUartNum))
        {
            if (paramCount >= 1)
            {
                uint8_t uartNum;
                if (!getParamValueU8t(params, paramCount, "v", uartNum))
                    result = ConfigResult::InvalidParameter;
                else
                    result = _configController->setNextionUartNum(uartNum);
            }
            else
            {
                result = ConfigResult::InvalidParameter;
            }
        }

        switch (result)
        {
        case ConfigResult::Success:
            sendAckOk(sender, command, &params[0]);
            break;
        case ConfigResult::InvalidParameter:
            sendAckErr(sender, command, F("Invalid parameter"), &params[0]);
            break;
        case ConfigResult::InvalidConfig:
            sendAckErr(sender, command, F("Config not available"), &params[0]);
            break;
        default:
            sendAckErr(sender, command, F("Unknown error"), &params[0]);
            break;
        }

        return true;
    }
};
