#pragma once

#include <Arduino.h>

class IWifiClient
{
public:
    virtual ~IWifiClient() = default;

    virtual bool connected() = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual size_t write(const uint8_t* buf, size_t size) = 0;
    virtual size_t write(uint8_t b) = 0;
    virtual void stop() = 0;
    virtual void flush() = 0;

    virtual int connect(const char* host, uint16_t port) = 0;

    virtual size_t print(const char* str) = 0;
    virtual size_t print(const __FlashStringHelper* str) = 0;
    virtual size_t print(char c) = 0;
    virtual size_t print(unsigned char b, int base = DEC) = 0;
    virtual size_t print(int n, int base = DEC) = 0;
    virtual size_t print(unsigned int n, int base = DEC) = 0;
    virtual size_t print(long n, int base = DEC) = 0;
    virtual size_t print(unsigned long n, int base = DEC) = 0;
    
    virtual size_t println(const char* str) = 0;
    virtual size_t println(const __FlashStringHelper* str) = 0;
    virtual size_t println(char c) = 0;
    virtual size_t println(unsigned char b, int base = DEC) = 0;
    virtual size_t println(int n, int base = DEC) = 0;
    virtual size_t println(unsigned int n, int base = DEC) = 0;
    virtual size_t println(long n, int base = DEC) = 0;
    virtual size_t println(unsigned long n, int base = DEC) = 0;
    virtual size_t println() = 0;

    virtual operator bool() = 0;
};
