

#include "SensorNetworkHandler.h"
#include "SystemDefinitions.h"


SensorNetworkHandler::SensorNetworkHandler(SensorController* sensorController)
	: _sensorController(sensorController)
{
}

CommandResult SensorNetworkHandler::handleRequest(const String& method,
	const String& command, StringKeyValue* params, uint8_t paramCount,
	char* responseBuffer, size_t bufferSize)
{
	(void)method;
	(void)params;
	(void)paramCount;

	String cmd = command;
	cmd.trim();

	if (cmd == SensorBearing)
	{
		// nothing to do in this context
	}

	formatJsonResponse(responseBuffer, bufferSize, false, "Invalid request");
	return CommandResult{ false, 0 };
}

void SensorNetworkHandler::formatStatusJson(char* buffer, size_t size)
{
	int written = snprintf(buffer, size, "\"sensor\":[");

	for (uint8_t i = 0; i < _sensorController->sensorCount(); i++)
	{
		BaseSensor* sensor = _sensorController->sensorGet(i);

		if (sensor == nullptr)
			continue;

		char sensorBuffer[MaximumJsonResponseBufferSize];
		sensorBuffer[0] = '\0';

		sensor->formatStatusJson(sensorBuffer, sizeof(sensorBuffer));

		// Add comma separator if not the first element
		if (i > 0 && written < (int)size)
		{
			written += snprintf(buffer + written, size - written, ",");
		}

		written += snprintf(buffer + written, size - written,
			"%s", sensorBuffer);
	}

	snprintf(buffer + written, size - written, "]");
}
