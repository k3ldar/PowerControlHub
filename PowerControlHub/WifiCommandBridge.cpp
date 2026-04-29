/*
 * PowerControlHub
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
#include "Local.h"
#include "WifiCommandBridge.h"
#include "SystemFunctions.h"

WifiCommandBridge::WifiCommandBridge()
    : _handlers(nullptr),
      _handlerCount(0),
      _captureStream(),
      _captureMgr(&_captureStream, nullptr)
{
}

void WifiCommandBridge::setHandlers(ISerialCommandHandler** handlers, uint8_t count)
{
    _handlers     = handlers;
    _handlerCount = count;
}

CommandResult WifiCommandBridge::handleRequest(const char* method,
    const char* command,
    StringKeyValue* params,
    uint8_t paramCount,
    char* responseBuffer,
    size_t bufferSize)
{
    if (!responseBuffer || bufferSize == 0)
    {
        return CommandResult::error(BufferInvalid);
    }

    if (!method || strcmp(method, "POST") != 0)
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":false,\"error\":\"Method not allowed, use POST\"");
        return CommandResult::error(InvalidCommandParameters);
    }

    if (!command || command[0] == '\0')
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":false,\"error\":\"No command specified\"");
        return CommandResult::error(InvalidCommandParameters);
    }

    if (!_handlers || _handlerCount == 0)
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":false,\"error\":\"No handlers registered\"");
        return CommandResult::error(InvalidCommandParameters);
    }

    // Find a handler that supports this command
    ISerialCommandHandler* target = nullptr;
    for (uint8_t i = 0; i < _handlerCount; i++)
    {
        if (_handlers[i] && _handlers[i]->supportsCommand(command))
        {
            target = _handlers[i];
            break;
        }
    }

    if (!target)
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":false,\"command\":\"%s\",\"error\":\"Unknown command\"", command);
        return CommandResult::error(InvalidCommandParameters);
    }

    // Reset the capture buffer, then invoke the handler using the capture manager as sender.
    // Any sendAckOk/sendAckErr calls will be written into _captureStream.
    // State-change commands (e.g. R0-R4) broadcast via BroadcastManager rather than sender,
    // so the capture buffer will remain empty on success — that is the expected signal.
    _captureStream.reset();
    target->handleCommand(&_captureMgr, command, params, paramCount);

    return buildJsonResponse(command, responseBuffer, bufferSize);
}

CommandResult WifiCommandBridge::buildJsonResponse(const char* command, char* responseBuffer, size_t bufferSize)
{
    const char* captured = _captureStream.getBuffer();

    // Empty capture means the handler used BroadcastManager for its success ACK.
    if (captured[0] == '\0')
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":true,\"command\":\"%s\"", command);
        return CommandResult::ok();
    }

    // Full wire format from BaseCommandHandler::sendAckOk/sendAckErr:
    //   ACK:cmd=ok\n
    //   ACK:cmd=ok:key=val;key=val\n   (with extra params)
    //   ACK:cmd=error message\n
    const char* ackPos = strstr(captured, "ACK:");
    if (!ackPos)
    {
        // Unexpected output — treat as success (non-ACK messages are debug/info)
        snprintf(responseBuffer, bufferSize,
            "\"success\":true,\"command\":\"%s\"", command);
        return CommandResult::ok();
    }

    // Move past "ACK:", then skip the echoed command token to find '='
    ackPos += 4;
    const char* eq = strchr(ackPos, '=');
    if (!eq)
    {
        snprintf(responseBuffer, bufferSize,
            "\"success\":true,\"command\":\"%s\"", command);
        return CommandResult::ok();
    }

    // Extract result string — terminated by ':', '\r', '\n', or end of string
    const char* resultStart = eq + 1;
    const char* p = resultStart;
    while (*p && *p != ':' && *p != '\r' && *p != '\n') p++;

    size_t resultLen = p - resultStart;
    char resultBuf[64];
    if (resultLen >= sizeof(resultBuf)) resultLen = sizeof(resultBuf) - 1;
    strncpy(resultBuf, resultStart, resultLen);
    resultBuf[resultLen] = '\0';

    // Locate the optional params block — everything after the second ':' on the ACK line
    // Format: ACK:cmd=ok:key=val;key=val;...\n
    const char* paramsStart = nullptr;
    if (*p == ':')
    {
        paramsStart = p + 1;
        // Trim any trailing \r\n so we don't include them in the last value
        size_t pLen = strlen(paramsStart);
        while (pLen > 0 && (paramsStart[pLen - 1] == '\r' || paramsStart[pLen - 1] == '\n'))
            pLen--;

        // Use a local copy with terminator stripped
        static char paramsBuf[CaptureStream::CaptureBufferSize];
        if (pLen >= sizeof(paramsBuf)) pLen = sizeof(paramsBuf) - 1;
        strncpy(paramsBuf, paramsStart, pLen);
        paramsBuf[pLen] = '\0';
        paramsStart = paramsBuf;
    }

    const bool success = (strcmp(resultBuf, "ok") == 0);

    // Build JSON into responseBuffer
    size_t pos = 0;

    // Start with mandatory fields
    int written = snprintf(responseBuffer + pos, bufferSize - pos,
        "\"success\":%s,\"command\":\"%s\"",
        success ? "true" : "false",
        command);

    if (written > 0)
        pos += (size_t)written;

    // Add error field on failure
    if (!success && pos < bufferSize)
    {
        char safeResult[64];
        SystemFunctions::sanitizeJsonString(resultBuf, safeResult, sizeof(safeResult));
        written = snprintf(responseBuffer + pos, bufferSize - pos,
            ",\"error\":\"%s\"", safeResult);

        if (written > 0)
            pos += (size_t)written;
    }

    // Parse and append any extra key=value params as individual JSON fields
    if (success && paramsStart && paramsStart[0] != '\0' && pos < bufferSize)
    {
        const char* tok = paramsStart;

        while (*tok != '\0' && pos < bufferSize)
        {
            // Find end of this param token (';' delimiter or end of string)
            const char* tokEnd = tok;
            while (*tokEnd && *tokEnd != ';') tokEnd++;

            size_t tokLen = tokEnd - tok;
            if (tokLen > 0)
            {
                // Split on '='
                const char* kvEq = (const char*)memchr(tok, '=', tokLen);
                if (kvEq)
                {
                    size_t keyLen = kvEq - tok;
                    size_t valLen = tokLen - keyLen - 1;

                    char key[DefaultMaxParamKeyLength + 1];
                    char val[DefaultMaxParamValueLength + 1];

                    if (keyLen >= sizeof(key))   keyLen = sizeof(key) - 1;
                    if (valLen >= sizeof(val))   valLen = sizeof(val) - 1;

                    strncpy(key, tok, keyLen);      key[keyLen] = '\0';
                    strncpy(val, kvEq + 1, valLen); val[valLen] = '\0';

                    char safeKey[DefaultMaxParamKeyLength + 1];
                    char safeVal[DefaultMaxParamValueLength + 1];
                    SystemFunctions::sanitizeJsonString(key, safeKey, sizeof(safeKey));
                    SystemFunctions::sanitizeJsonString(val, safeVal, sizeof(safeVal));

                    written = snprintf(responseBuffer + pos, bufferSize - pos,
                        ",\"%s\":\"%s\"", safeKey, safeVal);
                    if (written > 0) pos += (size_t)written;
                }
            }

            tok = (*tokEnd == ';') ? tokEnd + 1 : tokEnd;
        }
    }

    return success ? CommandResult::ok() : CommandResult::error(InvalidCommandParameters);
}
