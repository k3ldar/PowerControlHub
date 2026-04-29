#pragma once
/*
 * PowerControlHub
 * Copyright (C) 2026 Simon Carter (s1cart3r@gmail.com)
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
#include <Arduino.h>
#include "INetworkCommandHandler.h"
#include "RelayController.h"
#include "SensorController.h"
#include "ConfigManager.h"
#include "BaseSensor.h"
#include "SystemFunctions.h"

// ---------------------------------------------------------------------------
// Page-level fragments
// ---------------------------------------------------------------------------
// The head is split into two parts so we can inject the configured location
// name into the <title> at runtime.
constexpr char HtmlHeadPrefix[] =
	"<!DOCTYPE html><html lang=\"en\"><head>"
	"<meta charset=\"UTF-8\">"
	"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
	"<title>";

constexpr char HtmlHeadSuffix[] =
	"</title>"
	"<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\">"
	"<style>"
	"body{background:#ffffff;color:#000000}"
	".card{background:#16213e;border:1px solid #0f3460}"
	".card-header{background:#ffffff;font-weight:600;letter-spacing:.05em;text-transform:uppercase;font-size:.875em;color:#000000}"
	".sv{font-size:1.4rem;font-weight:700;color:#00d4ff}"
	".su{font-size:.8rem;color:#888;margin-left:2px}"
	".form-check-input:checked{background-color:#00d4ff;border-color:#00d4ff}"
	".form-check-input:focus{box-shadow:0 0 0 .25rem rgba(0,212,255,.25)}"
	"</style></head>"
	"<body class=\"p-3\">"
	"<div class=\"container-fluid\" style=\"max-width:700px\">"
	"<h5 class=\"text-center mb-4\" style=\"color:#00d4ff;letter-spacing:.1em\">&#9889; ";

constexpr char HtmlFoot[] =
	"<div class=\"text-center mt-3 small text-muted\">Last update: <span id=\"ts\">--</span></div>"
	"</div>"
	"<script>"
	"var $=x=>document.getElementById(x);"
	"function toggle(el,id){var prev=el.checked;el.disabled=true;fetch('/api/relay/R3?'+id+'='+(el.checked?1:0)).then(r=>r.json()).then(d=>{el.disabled=false;}).catch(()=>{el.checked=!prev;el.disabled=false;});}"
	"function refresh(){"
	"fetch('/api/index').then(r=>r.json()).then(d=>{"
	"if(d.relays){var slot=1;d.relays.forEach(r=>{if(r.pin!=255){var e=$('sw'+slot);if(e)e.checked=(r.state==1);slot++;}});}"
	"if(d.sensors){var sensorSlot=1;for(var key in d.sensors){var sensorData=d.sensors[key];var el=$('s'+sensorSlot);if(el){var val='--';"
	"if(sensorData.temperature!==undefined)val=sensorData.temperature.toFixed(1)+'°C';else if(sensorData.humidity!==undefined)val=sensorData.humidity+'%';"
	"else if(sensorData.gps&&sensorData.gps.lat!==undefined)val=sensorData.gps.lat.toFixed(2)+','+sensorData.gps.lon.toFixed(2);else if(sensorData.name!==undefined)"
	"{var json=JSON.stringify(sensorData);var match=json.match(/:(\\d+\\.?\\d*)/);if(match)val=match[1];}el.innerHTML=val;}sensorSlot++;}}"
	"$('ts').textContent=new Date().toLocaleTimeString();"
	"}).catch(()=>{});}"
	"setInterval(refresh,5000);refresh();"
	"</script></body></html>";

// ---------------------------------------------------------------------------
// Controls section wrappers
// ---------------------------------------------------------------------------
constexpr char SwitchSectionStart[] =
	"<div class=\"card mb-4\">"
	"<div class=\"card-header\">Controls</div>"
	"<div class=\"card-body\" style=\"color:white;\"><div class=\"row g-3\">";

constexpr char SwitchSectionEnd[] = "</div></div></div>";

constexpr char SwitchSectionA[] = "<div class=\"col-6 col-sm-3 d-flex align-items-center justify-content-between gap-2\">"
							  "<label class=\"form-check-label\" for=\"sw";
constexpr char SwitchSectionB[] = "\">";
constexpr char SwitchSectionC[] = "</label><div class=\"form-check form-switch mb-0\">"
							  "<input class=\"form-check-input\" type=\"checkbox\" role=\"switch\" id=\"sw";
constexpr char SwitchSectionD[] = "\" onchange=\"toggle(this,";
constexpr char SwitchSectionE[] = ")\"></div></div>";

// ---------------------------------------------------------------------------
// Sensors section wrappers
// ---------------------------------------------------------------------------
constexpr char SensorSectionStart[] =
	"<div class=\"card\">"
	"<div class=\"card-header\">Sensors</div>"
	"<div class=\"card-body p-2\"><div class=\"row g-2\">";

constexpr char SensorSectionEnd[] = "</div></div></div>";

constexpr char SensorSectionA[] = "<div class=\"col-6 col-sm-3\"><div class=\"card h-100 text-center p-2\" style=\"background:#0d1b2a;color:white\">"
							 "<div class=\"small mb-1\">";
constexpr char SensorSectionB[] = "</div><div><span class=\"sv\" id=\"s";
constexpr char SensorSectionC[] = "\">";
constexpr char SensorSectionD[] = "</span></div></div></div>";

// ---------------------------------------------------------------------------
// WebIndexNetworkHandler
// ---------------------------------------------------------------------------
class WebIndexNetworkHandler : public INetworkCommandHandler
{
private:
	RelayController*    _relayController;
	SensorController*   _sensorController;

	// Writes sensor value(s) from a BaseSensor to the client by calling its formatStatusJson.
	// Extracts and displays the first meaningful value from the sensor's JSON output.
	void printSensorValues(IWifiClient& client, BaseSensor* sensor, uint8_t slotId) const
	{
		(void)slotId; // Not used, but kept for future extensibility

		if (!sensor)
		{
			client.print("--");
			return;
		}

		char sensorBuffer[MaximumJsonResponseBufferSize];
		sensorBuffer[0] = '\0';
		sensor->formatStatusJson(sensorBuffer, sizeof(sensorBuffer));

		// Extract and display first numeric value or string from JSON
		// This is a simplified approach - shows raw sensor data
		const char* ptr = sensorBuffer;
		bool foundValue = false;

		// Buffer to collect the value (HTML-escape it)
		char valueBuffer[32];
		int valuePos = 0;

		while (*ptr && !foundValue)
		{
			if (*ptr == ':')
			{
				ptr++;
				while (*ptr == ' ' || *ptr == '"') ptr++;

				// Collect up to 30 chars or until comma/brace/quote
				valuePos = 0;
				while (*ptr && *ptr != ',' && *ptr != '}' && *ptr != '"' && valuePos < 30)
				{
					valueBuffer[valuePos++] = *ptr;
					ptr++;
				}
				valueBuffer[valuePos] = '\0';
				foundValue = true;
			}
			else
			{
				ptr++;
			}
		}

		if (foundValue && valuePos > 0)
		{
			// HTML-escape the value before displaying
			char escapedValue[64];
			SystemFunctions::escapeHtml(valueBuffer, escapedValue, sizeof(escapedValue));
			client.print(escapedValue);
		}
		else
		{
			client.print("--");
		}
	}

	// Writes one enabled toggle-switch column to the client.
	// slotId is the 1-based display index used for HTML element IDs.
	void buildSwitch(IWifiClient& client, uint8_t relayIndex, uint8_t slotId) const
	{
		bool isOn = false;
		if (_relayController)
		{
			CommandResult r = _relayController->getRelayStatus(relayIndex);
			isOn = r.success && r.status == 1;
		}

		Config* config = ConfigManager::getConfigPtr();
		const char* name = (config != nullptr) ? config->relay.relays[relayIndex].longName : "Relay";

		// HTML-escape the relay name for safe display
		char escapedName[64];
		SystemFunctions::escapeHtml(name, escapedName, sizeof(escapedName));

		client.print(SwitchSectionA);
		client.print(slotId);
		client.print(SwitchSectionB);
		client.print(escapedName);
		client.print(SwitchSectionC);
		client.print(slotId);
		client.print(SwitchSectionD);
		client.print(relayIndex);   // zero-based relay index used by /api/relay
		client.print(SwitchSectionE);

		// Pre-check the toggle if the relay is currently on
		if (isOn)
		{
			// Inject checked attribute via a small inline script referencing the element
			client.print("<script>document.getElementById('sw");
			client.print(slotId);
			client.print("').checked=true;</script>");
		}
	}

	// Writes one enabled sensor tile to the client.
	// slotId is the 1-based display index used for HTML element IDs.
	void buildSensor(IWifiClient& client, BaseSensor* sensor, uint8_t slotId) const
	{
		// HTML-escape the sensor name for safe display
		char escapedName[64];
		SystemFunctions::escapeHtml(sensor->getSensorName(), escapedName, sizeof(escapedName));

		client.print(SensorSectionA);
		client.print(escapedName);
		client.print(SensorSectionB);
		client.print(slotId);
		client.print(SensorSectionC);
		printSensorValues(client, sensor, slotId);
		client.print(SensorSectionD);
	}

	// Sends the complete HTTP response with HTML content-type.
	// Content-Length is omitted because the page is streamed and its size is not
	// known in advance; the browser will detect end-of-body on connection close.
	void sendHtmlHeaders(IWifiClient& client, bool isPersistent) const
	{
		client.print(F("HTTP/1.1 200 OK\r\n"));
		client.print(F("Content-Type: text/html; charset=UTF-8\r\n"));
		if (isPersistent)
		{
			client.print(F("Connection: keep-alive\r\n"));
		}
		else
		{
			client.print(F("Connection: close\r\n"));
		}
		client.print(F("\r\n"));
	}

public:
	WebIndexNetworkHandler(RelayController* relayController, SensorController* sensorController)
		: _relayController(relayController), _sensorController(sensorController)
	{
	}

	~WebIndexNetworkHandler() override = default;

	const char* getRoute() const override { return "/index"; }

	void formatWifiStatusJson(IWifiClient* /*client*/) override
	{
		// Not included in the /api/index JSON status response
	}

	CommandResult handleRequest(const char* /*method*/,
		const char* /*command*/,
		StringKeyValue* /*params*/,
		uint8_t /*paramCount*/,
		char* responseBuffer,
		size_t bufferSize) override
	{
		// HTML responses are delivered via generateHtml(); this path is never reached
		// for normal requests but is provided to satisfy the interface.
		formatJsonResponse(responseBuffer, bufferSize, false, "Use GET /index for HTML");
		return CommandResult::error(0);
	}

	// Streams the full HTML page directly to the WiFi client.
	bool generateHtml(IWifiClient& client) const override
	{
		sendHtmlHeaders(client, false);

		// Print head prefix, then configured location name (or default), then suffix
		client.print(HtmlHeadPrefix);

		Config* config = ConfigManager::getConfigPtr();
		if (config != nullptr && config->location.name[0] != '\0')
		{
			// HTML-escape the location name for safe display in title and heading
			char escapedLocation[64];
			SystemFunctions::escapeHtml(config->location.name, escapedLocation, sizeof(escapedLocation));
			client.print(escapedLocation);
		}
		else
		{
			client.print("ESP32 Control Panel");
		}

		client.print(HtmlHeadSuffix);

		// --- Controls section (enabled relays only) ---
		bool hasRelays = false;
		if (_relayController && config != nullptr)
		{
			uint8_t count = _relayController->getRelayCount();
			for (uint8_t i = 0; i < count; i++)
			{
				if (!_relayController->isDisabled(i))
				{
					hasRelays = true;
					break;
				}
			}
		}

		if (hasRelays)
		{
			client.print(SwitchSectionStart);
			uint8_t slotId = 1;
			uint8_t count = _relayController->getRelayCount();
			for (uint8_t i = 0; i < count; i++)
			{
				if (!_relayController->isDisabled(i))
				{
					buildSwitch(client, i, slotId++);
				}
			}
			client.print(SwitchSectionEnd);
		}

		// --- Sensors section (all sensors from SensorController) ---
		bool hasSensors = false;
		if (_sensorController && _sensorController->sensorCount() > 0)
		{
			hasSensors = true;
		}

		if (hasSensors)
		{
			client.print(SensorSectionStart);
			uint8_t slotId = 1;
			for (uint8_t i = 0; i < _sensorController->sensorCount(); i++)
			{
				BaseSensor* sensor = _sensorController->sensorGet(i);
				if (sensor != nullptr)
				{
					buildSensor(client, sensor, slotId++);
				}
			}
			client.print(SensorSectionEnd);
		}

		client.print(HtmlFoot);

		return true;
	}
};
