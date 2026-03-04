#pragma once

#include "IWifiClient.h"

class NullWifiClient : public IWifiClient
{
public:
    bool connected() override
    {
        return false;
    }
    
    int available() override
    {
        return 0;
    }
    
    int read() override
    {
        return -1;
    }
    
    int peek() override
    {
        return -1;
    }
    
    size_t write(const uint8_t* buf, size_t size) override
    {
        (void)buf;
        (void)size;
        return 0;
    }
    
    size_t write(uint8_t b) override
    {
        (void)b;
        return 0;
    }
    
    void stop() override
    {
    }
    
    void flush() override
    {
    }

    int connect(const char* host, uint16_t port) override
    {
        (void)host;
        (void)port;
        return 0;
    }

    size_t print(const char* str) override
    {
        (void)str;
        return 0;
    }
    
    size_t print(const __FlashStringHelper* str) override
    {
        (void)str;
        return 0;
    }
    
    size_t print(char c) override
    {
        (void)c;
        return 0;
    }
    
    size_t print(unsigned char b, int base = DEC) override
    {
        (void)b;
        (void)base;
        return 0;
    }
    
    size_t print(int n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }
    
    size_t print(unsigned int n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }
    
    size_t print(long n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }
    
    size_t print(unsigned long n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }
    
    size_t println(const char* str) override
    {
        (void)str;
        return 0;
    }

    size_t println(const __FlashStringHelper* str) override
    {
        (void)str;
        return 0;
    }

    size_t println(char c) override
    {
        (void)c;
        return 0;
    }

    size_t println(unsigned char b, int base = DEC) override
    {
        (void)b;
        (void)base;
        return 0;
    }

    size_t println(int n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }

    size_t println(unsigned int n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }

    size_t println(long n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }

    size_t println(unsigned long n, int base = DEC) override
    {
        (void)n;
        (void)base;
        return 0;
    }

    size_t println() override
    {
        return 0;
    }
    
    operator bool() const override
    {
        return false;
    }
};
