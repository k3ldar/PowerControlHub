#pragma once

#include <WiFiClient.h>

class NetworkJsonVisitor
{
public:
	virtual void formatWifiStatusJson(WiFiClient* client) = 0;
	virtual ~NetworkJsonVisitor() = default;
};