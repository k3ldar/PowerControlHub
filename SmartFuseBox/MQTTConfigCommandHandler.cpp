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
#include "Local.h"

#include "MQTTConfigCommandHandler.h"
#include "MQTTController.h"

#include "ConfigManager.h"
#include "SystemFunctions.h"
#include <SerialCommandManager.h>
#include <string.h>
#include <stdlib.h>

MQTTConfigCommandHandler::MQTTConfigCommandHandler(ConfigController* configController, MQTTController* mqttController, SerialCommandManager* commandMgr)
    : _configController(configController)
    , _mqttController(mqttController)
    , _config(nullptr)
    , _commandMgr(commandMgr)
{
    if (_configController != nullptr)
    {
        _config = _configController->getConfigPtr();
    }
}

bool MQTTConfigCommandHandler::processCommand(const char* command, const char* params)
{
    if (command == nullptr)
    {
        return false;
    }
    
#if defined(MQTT_SUPPORT)
    // M0 - MQTT Enabled
    if (SystemFunctions::commandMatches(command, MqttConfigEnable))
    {
        return handleMqttEnable(params);
    }
    // M1 - MQTT Broker
    else if (SystemFunctions::commandMatches(command, MqttConfigBroker))
    {
        return handleMqttBroker(params);
    }
    // M2 - MQTT Port
    else if (SystemFunctions::commandMatches(command, MqttConfigPort))
    {
        return handleMqttPort(params);
    }
    // M3 - MQTT Username
    else if (SystemFunctions::commandMatches(command, MqttConfigUsername))
    {
        return handleMqttUsername(params);
    }
    // M4 - MQTT Password
    else if (SystemFunctions::commandMatches(command, MqttConfigPassword))
    {
        return handleMqttPassword(params);
    }
    // M5 - MQTT Device ID
    else if (SystemFunctions::commandMatches(command, MqttConfigDeviceId))
    {
        return handleMqttDeviceId(params);
    }
    // M6 - HA Discovery Enabled
    else if (SystemFunctions::commandMatches(command, MqttConfigHADiscovery))
    {
        return handleMqttHADiscovery(params);
    }
    // M7 - Keep Alive Interval
    else if (SystemFunctions::commandMatches(command, MqttConfigKeepAlive))
    {
        return handleMqttKeepAlive(params);
    }
    // M8 - MQTT Connection State (query only)
    else if (SystemFunctions::commandMatches(command, MqttConfigState))
    {
        return handleMqttState(params);
    }
    // M9 - Discovery Prefix
    else if (SystemFunctions::commandMatches(command, MqttConfigDiscoveryPrefix))
    {
        return handleMqttDiscoveryPrefix(params);
    }
#else
    (void)command;
    (void)params;
#endif

    return false;
}

bool MQTTConfigCommandHandler::handleMqttEnable(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }
    
    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }
    
    // Parse value
	bool enabled = SystemFunctions::parseBooleanValue(params);
    
    _config->mqtt.enabled = enabled;
    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttBroker(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }

    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }

    // Set broker address (value is already extracted)
    strncpy(_config->mqtt.broker, params, ConfigMqttBrokerLength - 1);
    _config->mqtt.broker[ConfigMqttBrokerLength - 1] = '\0';

    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttPort(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }
    
    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }
    
    // Parse port
    uint16_t port;
    if (!parseUint16(params, &port) || port == 0)
    {
        return false;
    }
    
    _config->mqtt.port = port;
    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttUsername(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }

    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }

    // Set username (value is already extracted)
    strncpy(_config->mqtt.username, params, ConfigMqttUsernameLength - 1);
    _config->mqtt.username[ConfigMqttUsernameLength - 1] = '\0';

    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttPassword(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }

    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }

    // Set password (value is already extracted)
    strncpy(_config->mqtt.password, params, ConfigMqttPasswordLength - 1);
    _config->mqtt.password[ConfigMqttPasswordLength - 1] = '\0';

    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttDeviceId(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }

    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }

    // Set device ID (value is already extracted)
    strncpy(_config->mqtt.deviceId, params, ConfigMqttDeviceIdLength - 1);
    _config->mqtt.deviceId[ConfigMqttDeviceIdLength - 1] = '\0';

    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttHADiscovery(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }
    
    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }
    
    // Parse value
    bool enabled = SystemFunctions::parseBooleanValue(params);
    
    _config->mqtt.useHomeAssistantDiscovery = enabled;
    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttKeepAlive(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }
    
    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }
    
    // Parse keep-alive
    uint16_t keepAlive;
    if (!parseUint16(params, &keepAlive) || keepAlive == 0)
    {
        return false;
    }
    
    _config->mqtt.keepAliveInterval = keepAlive;
    return true;
#else
    (void)params;
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttState(const char* params)
{
    (void)params;
#if defined(MQTT_SUPPORT)

    if (_config == nullptr)
    {
        return false;
    }

    // Return actual MQTT connection state if controller is available
    if (_mqttController != nullptr)
    {
        return _mqttController->isConnected();
    }

    // Fallback to config enabled state if controller not available
    return _config->mqtt.enabled;
#else
    return false;
#endif
}

bool MQTTConfigCommandHandler::handleMqttDiscoveryPrefix(const char* params)
{
#if defined(MQTT_SUPPORT)
    if (_config == nullptr)
    {
        return false;
    }

    if (params == nullptr || params[0] == '\0')
    {
        return false;
    }

    // Set discovery prefix (value is already extracted)
    strncpy(_config->mqtt.discoveryPrefix, params, ConfigMqttDiscoveryPrefixLength - 1);
    _config->mqtt.discoveryPrefix[ConfigMqttDiscoveryPrefixLength - 1] = '\0';

    return true;
#else
    (void)params;
    return false;
#endif
}

// ============================================================================
// Private Helper Methods
// ============================================================================
bool MQTTConfigCommandHandler::parseUint16(const char* value, uint16_t* result)
{
    if (value == nullptr || result == nullptr || value[0] == '\0')
    {
        return false;
    }

    // Value is already extracted by SerialCommandManager/WifiServer
    char* endPtr;
    long parsed = strtol(value, &endPtr, 10);

    if (endPtr == value || parsed < 0 || parsed > 65535)
    {
        return false;
    }

    *result = (uint16_t)parsed;
    return true;
}
