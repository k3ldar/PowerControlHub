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

#include "Local.h"

#if defined(WIFI_SUPPORT) && defined(ARDUINO_UNO_R4)

#include <WiFiS3.h>
#include "IWifiClient.h"

class R4WifiClient : public IWifiClient
{
private:
    WiFiClient _client;
    
public:
    R4WifiClient() : _client()
    {
    }
    
    explicit R4WifiClient(WiFiClient client) : _client(client)
    {
    }
    
    bool connected() override
    {
        return _client.connected();
    }
    
    int available() override
    {
        return _client.available();
    }
    
    int read() override
    {
        return _client.read();
    }
    
    int peek() override
    {
        return _client.peek();
    }
    
    size_t write(const uint8_t* buf, size_t size) override
    {
        return _client.write(buf, size);
    }
    
    size_t write(uint8_t b) override
    {
        return _client.write(b);
    }
    
    void stop() override
    {
        _client.stop();
    }
    
    void flush() override
    {
        _client.flush();
    }

    int connect(const char* host, uint16_t port) override
    {
        return _client.connect(host, port);
    }

    size_t print(const char* str) override
    {
        return _client.print(str);
    }
    
    size_t print(const __FlashStringHelper* str) override
    {
        return _client.print(str);
    }
    
    size_t print(char c) override
    {
        return _client.print(c);
    }
    
    size_t print(unsigned char b, int base = DEC) override
    {
        return _client.print(b, base);
    }
    
    size_t print(int n, int base = DEC) override
    {
        return _client.print(n, base);
    }
    
    size_t print(unsigned int n, int base = DEC) override
    {
        return _client.print(n, base);
    }
    
    size_t print(long n, int base = DEC) override
    {
        return _client.print(n, base);
    }
    
    size_t print(unsigned long n, int base = DEC) override
    {
        return _client.print(n, base);
    }
    
    size_t println(const char* str) override
    {
        return _client.println(str);
    }

    size_t println(const __FlashStringHelper* str) override
    {
        return _client.println(str);
    }

    size_t println(char c) override
    {
        return _client.println(c);
    }

    size_t println(unsigned char b, int base = DEC) override
    {
        return _client.println(b, base);
    }

    size_t println(int n, int base = DEC) override
    {
        return _client.println(n, base);
    }

    size_t println(unsigned int n, int base = DEC) override
    {
        return _client.println(n, base);
    }

    size_t println(long n, int base = DEC) override
    {
        return _client.println(n, base);
    }

    size_t println(unsigned long n, int base = DEC) override
    {
        return _client.println(n, base);
    }

    size_t println() override
    {
        return _client.println();
    }
    
    operator bool() override
    {
        return _client;
    }
};
#endif