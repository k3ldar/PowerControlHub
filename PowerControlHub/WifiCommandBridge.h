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
#pragma once

#include <Arduino.h>
#include <SerialCommandManager.h>
#include "INetworkCommandHandler.h"

/**
 * @brief A RAM-backed Arduino Stream that captures all bytes written to it.
 * 
 * Used as the backing stream for a SerialCommandManager so that the ACK
 * text produced by any ISerialCommandHandler can be intercepted without
 * touching the handler or the library.
 */
class CaptureStream : public Stream
{
public:
    static constexpr size_t CaptureBufferSize = 256;

    CaptureStream() : _len(0) { _buffer[0] = '\0'; }

    size_t write(uint8_t byte) override
    {
        if (_len < CaptureBufferSize)
        {
            _buffer[_len++] = static_cast<char>(byte);
            _buffer[_len] = '\0';
        }
        return 1;
    }

    size_t write(const uint8_t* buf, size_t size) override
    {
        for (size_t i = 0; i < size; i++) write(buf[i]);
        return size;
    }

    int available() override { return 0; }
    int read()      override { return -1; }
    int peek()      override { return -1; }
    void flush()    override {}

    void reset() { _len = 0; _buffer[0] = '\0'; }
    const char* getBuffer() const { return _buffer; }

private:
    char   _buffer[CaptureBufferSize + 1];
    size_t _len;
};

/**
 * @brief WiFi transport bridge for all ISerialCommandHandler implementations.
 * 
 * Exposes the endpoint POST /api/command/{cmd} over HTTP.  The POST body
 * must contain newline-delimited key=value parameter pairs, e.g.:
 *
 *   POST /api/command/R3
 *   3=1
 *
 *   POST /api/command/S1
 *   i=3
 *   t=5
 *   o0=100
 *
 * The bridge finds the registered ISerialCommandHandler that supports the
 * given command, creates a fake SerialCommandManager backed by a
 * CaptureStream, calls handleCommand() on that handler, then converts the
 * captured ACK text back to JSON.
 *
 * This means every command has exactly ONE implementation — the existing
 * serial handler — regardless of whether it is invoked via serial or WiFi.
 */
class WifiCommandBridge : public INetworkCommandHandler
{
public:
    WifiCommandBridge();

    /**
     * @brief Register the serial command handlers this bridge should delegate to.
     * 
     * Must be called before any HTTP request is processed.
     * 
     * @param handlers  Pointer to array of ISerialCommandHandler pointers.
     * @param count     Number of handlers in the array.
     */
    void setHandlers(ISerialCommandHandler** handlers, uint8_t count);

    // INetworkCommandHandler
    const char* getRoute() const override { return "/api/command"; }

    CommandResult handleRequest(const char* method,
        const char* command,
        StringKeyValue* params,
        uint8_t paramCount,
        char* responseBuffer,
        size_t bufferSize) override;

    // NetworkJsonVisitor — bridge has no status to contribute to /api/index
    void formatWifiStatusJson(IWifiClient* client) override {}

private:
    ISerialCommandHandler** _handlers;
    uint8_t                 _handlerCount;

    // CaptureStream MUST be declared before _captureMgr so it is initialised first.
    CaptureStream      _captureStream;
    SerialCommandManager _captureMgr;

    /**
     * @brief Convert the captured ACK text (or an empty buffer) to a JSON fragment.
     * 
     * Rules:
     *   - Empty buffer  → success  (state-change commands broadcast via BroadcastManager, not sender)
     *   - ACK:cmd=ok    → success
     *   - ACK:cmd=<msg> → failure with error message
     * 
     * @param command        Original command token, included in the JSON.
     * @param responseBuffer Destination buffer.
     * @param bufferSize     Size of destination buffer.
     * @return CommandResult reflecting the parsed outcome.
     */
    CommandResult buildJsonResponse(const char* command, char* responseBuffer, size_t bufferSize);
};
