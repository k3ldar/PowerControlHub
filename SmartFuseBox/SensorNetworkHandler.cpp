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
#include "SensorNetworkHandler.h"
#include "SystemDefinitions.h"


SensorNetworkHandler::SensorNetworkHandler(SensorController* sensorController)
	: _sensorController(sensorController)
{
}

CommandResult SensorNetworkHandler::handleRequest(const char* method,
	const char* command, StringKeyValue* params, uint8_t paramCount,
	char* responseBuffer, size_t bufferSize)
{
	(void)method;
	(void)params;
	(void)paramCount;
	(void)responseBuffer;
	(void)bufferSize;

	if (strcmp(command, SensorBearing) == 0)
	{
		// nothing to do in this context
	}

    return CommandResult::error(InvalidCommandParameters);
}

void SensorNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
    if (!buffer || size == 0)
    {
        return;
    }

    int written = snprintf(buffer, size, "\"sensors\":{");
    if (written < 0 || written >= static_cast<int>(size))
    {
        buffer[size - 1] = '\0';
        return; // Buffer too small for even the opening
    }

    bool firstSensor = true;
    for (uint8_t i = 0; i < _sensorController->sensorCount(); i++)
    {
        BaseSensor* sensor = _sensorController->sensorGet(i);

        if (sensor == nullptr)
            continue;

        char sensorBuffer[MaximumJsonResponseBufferSize];
        sensorBuffer[0] = '\0';

        sensor->formatStatusJson(sensorBuffer, sizeof(sensorBuffer));

        if (sensorBuffer[0] == '\0')
            continue;

        // Add comma separator if not the first element
        if (!firstSensor)
        {
            int n = snprintf(buffer + written, size - written, ",");
            if (n < 0 || written + n >= static_cast<int>(size))
            {
                buffer[size - 1] = '\0';
                return; // Out of space
            }
            written += n;
        }

        // Write sensor entry (removed duplicate and fixed format)
        int n = snprintf(buffer + written, size - written, 
            "\"%s\":{\"id\":%d,\"type\":%d,%s}",
            sensor->getSensorName(), 
            static_cast<uint8_t>(sensor->getSensorId()), 
            static_cast<uint8_t>(sensor->getSensorType()), 
            sensorBuffer);

        if (n < 0 || written + n >= static_cast<int>(size))
        {
            buffer[size - 1] = '\0';
            return; // Out of space
        }
        written += n;
        firstSensor = false;
    }

    // Close JSON object
    int n = snprintf(buffer + written, size - written, "}");
    if (n < 0 || written + n >= static_cast<int>(size))
    {
        buffer[size - 1] = '\0';
    }
}

void SensorNetworkHandler::formatWifiStatusJson(IWifiClient* client)
{
    char buffer[MaximumJsonResponseBufferSize];
    buffer[0] = '\0';

    formatStatusJson(buffer, sizeof(buffer));
    client->print(buffer);
}
