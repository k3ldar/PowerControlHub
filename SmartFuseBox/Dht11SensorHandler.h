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
#pragma once


#include <Arduino.h>

#include "Local.h"
#include "SystemDefinitions.h"
#include "WarningManager.h"
#include "WarningType.h"
#include "BaseSensor.h"
#include "Environment.h"

constexpr uint64_t TempHumidityCheckMs = 2500;

/**
 * @brief Sensor handler for DHT11 monitoring.
 *
 * Reads temperature and humidity sensor values, 
 * and reports readings to both link and computer serial connections.
 */
class Dht11SensorHandler : public BaseSensor, public BroadcastLoggerSupport
{
private:
	SensorCommandHandler* _sensorCommandHandler;
	WarningManager* _warningManager;
	const uint8_t _sensorPin;
	float _humidityOffset;
	float _temperatureOffset;
	float _humidity;
	float _celsius;

#if defined(MQTT_SUPPORT)
	char _slugTemp[32];
	char _slugHumidity[32];
	char _slugDewPoint[32];
	char _slugComfort[32];
	char _slugCondensation[40];
	char _nameTemp[48];
	char _nameHumidity[48];
	char _nameDewPoint[48];
	char _nameComfort[48];
	char _nameCondensation[48];
#endif

protected:
	// ── Return codes ─────────────────────────────────────────────────────────
	static constexpr int Dht11Ok       =  0;
	static constexpr int Dht11Timeout  = -1;
	static constexpr int Dht11Checksum = -2;

	// ── DHT11 one-wire protocol constants ────────────────────────────────────

	// Spec: host pulls bus LOW for a minimum of 18 ms to trigger the sensor.
	static constexpr uint8_t Dht11StartLowMs        = 18;

	// Spec: host drives HIGH for 20–40 µs before releasing to INPUT so the
	// sensor can detect the rising edge.  40 µs satisfies the upper bound.
	static constexpr uint8_t Dht11StartHighUs        = 40;

	// Spec: 40 bits (5 bytes) per reading, transmitted MSB-first within each byte.
	static constexpr uint8_t Dht11BitCount           = 40;
	static constexpr uint8_t Dht11ByteCount          = 5;
	static constexpr uint8_t Dht11BitsPerByte        = 8;

	// Spec: each bit is preceded by a 50 µs LOW separator, then a HIGH pulse:
	//   bit '0' → 26–28 µs HIGH
	//   bit '1' → 70 µs HIGH
	// 40 µs sits unambiguously between the two ranges and is the standard threshold.
	static constexpr uint8_t Dht11BitOneThresholdUs  = 40;

	// Spec: byte layout of the 5 transmitted data bytes.
	static constexpr uint8_t Dht11HumIntIdx          = 0; // humidity    integer part
	static constexpr uint8_t Dht11HumDecIdx          = 1; // humidity    decimal part (always 0 on DHT11)
	static constexpr uint8_t Dht11TmpIntIdx          = 2; // temperature integer part
	static constexpr uint8_t Dht11TmpDecIdx          = 3; // temperature decimal part (always 0 on DHT11)
	static constexpr uint8_t Dht11ChecksumIdx        = 4; // checksum = (byte0+byte1+byte2+byte3) & 0xFF

	// Spec: decimal byte encodes tenths (e.g. decimal byte 5 → +0.5 °C / +0.5 %).
	static constexpr float   Dht11DecimalScale       = 0.1f;

	// Iteration cap for every bounded spin-wait in readDht11().
	// digitalRead() on ESP32 at 240 MHz takes ~0.5–1 µs, so 10 000 iterations
	// gives a ~5–10 ms hard ceiling per transition — well above any legitimate
	// DHT11 signal period (max 80 µs response, 70 µs bit-HIGH) yet fast enough
	// to recover from a fault without stalling the main loop.
	static constexpr unsigned int Dht11MaxLoopCount  = 10000;

	/**
	 * @brief Read temperature and humidity from a DHT11 sensor.
	 *
	 * Every spin-wait is bounded by Dht11MaxLoopCount so the function can never
	 * hang, regardless of sensor state or electrical faults.  The 18 ms host
	 * start-signal uses delay() which on ESP32/FreeRTOS calls vTaskDelay(),
	 * yielding to the scheduler — not a hard block.
	 *
	 * Adapted from https://github.com/adidax/dht11 which is distributed under GPL v3
	 *
	 * @param pin             GPIO data pin connected to the DHT11.
	 * @param outTemperature  Receives temperature in °C on success.
	 * @param outHumidity     Receives relative humidity (%) on success.
	 * @return Dht11Ok, Dht11Timeout, or Dht11Checksum.
	 */
	static int readDht11(uint8_t pin, float& outTemperature, float& outHumidity)
	{
		uint8_t bits[Dht11ByteCount] = {};
		uint8_t cnt = Dht11BitsPerByte - 1;   // start at MSB (bit 7)
		uint8_t idx = 0;

		// Host start signal: pull LOW ≥ Dht11StartLowMs then drive HIGH briefly before releasing
		pinMode(pin, OUTPUT);
		digitalWrite(pin, LOW);
		delay(Dht11StartLowMs);
		digitalWrite(pin, HIGH);
		delayMicroseconds(Dht11StartHighUs);
		pinMode(pin, INPUT);

		// Sensor acknowledgement (80 µs LOW + 80 µs HIGH) — every loop is bounded
		unsigned int loopCnt = Dht11MaxLoopCount;

		while (digitalRead(pin) == LOW)
			if (loopCnt-- == 0) return Dht11Timeout;

		loopCnt = Dht11MaxLoopCount;

		while (digitalRead(pin) == HIGH)
			if (loopCnt-- == 0) return Dht11Timeout;

		// Read Dht11BitCount bits; each bit = 50 µs LOW separator + variable-width HIGH
		for (int i = 0; i < Dht11BitCount; i++)
		{
			loopCnt = Dht11MaxLoopCount;
			while (digitalRead(pin) == LOW)
				if (loopCnt-- == 0) return Dht11Timeout;

			unsigned long t = micros();

			loopCnt = Dht11MaxLoopCount;
			while (digitalRead(pin) == HIGH)
				if (loopCnt-- == 0) return Dht11Timeout;

			// HIGH pulse longer than Dht11BitOneThresholdUs µs decodes as logic '1'
			if ((micros() - t) > Dht11BitOneThresholdUs)
				bits[idx] |= (1 << cnt);

			if (cnt == 0)
			{
				cnt = Dht11BitsPerByte - 1;
				idx++;
			}
			else cnt--;
		}

		// Verify checksum: low byte of the sum of the four data bytes
		uint8_t sum = (bits[Dht11HumIntIdx] + bits[Dht11HumDecIdx]
					 + bits[Dht11TmpIntIdx]  + bits[Dht11TmpDecIdx]) & 0xFF;

		if (bits[Dht11ChecksumIdx] != sum)
			return Dht11Checksum;

		outHumidity    = bits[Dht11HumIntIdx] + (bits[Dht11HumDecIdx] * Dht11DecimalScale);
		outTemperature = bits[Dht11TmpIntIdx]  + (bits[Dht11TmpDecIdx] * Dht11DecimalScale);

		return Dht11Ok;
	}

	void initialize() override
	{
	}

	uint64_t update() override
	{
		sendDebug("Reading DHT11 sensor...", _name);

		float temperature = 0.0f;
		float humidity    = 0.0f;
		int result = readDht11(_sensorPin, temperature, humidity);

		if (result != Dht11Ok)
		{
			if (_warningManager && !_warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
			{
				_warningManager->raiseWarning(WarningType::TemperatureSensorFailure);
				char buffer[48];
				const char* errStr = (result == Dht11Checksum) ? "checksum failure" : "timeout";
				snprintf(buffer, sizeof(buffer), "DHT11 read error: %s", errStr);
				sendError(buffer, "DHT11 Error");
			}
			return TempHumidityCheckMs;
		}

		if (_warningManager && _warningManager->isWarningActive(WarningType::TemperatureSensorFailure))
			_warningManager->clearWarning(WarningType::TemperatureSensorFailure);

		_humidity = humidity + _humidityOffset;
		_celsius  = temperature + _temperatureOffset;

		StringKeyValue params[2];
		strncpy(params[0].key, ValueParamName, sizeof(params[0].key));
		strncpy(params[1].key, "n", sizeof(params[1].key));
		strncpy(params[1].value, _name, sizeof(params[1].value) - 1);
		params[1].value[sizeof(params[1].value) - 1] = '\0';
		snprintf(params[0].value, sizeof(params[0].value), "%.1f", _celsius);
		sendCommand(SensorTemperature, params, 2);
		snprintf(params[0].value, sizeof(params[0].value), "%.1f", _humidity);
		sendCommand(SensorHumidity, params, 2);

		if (_sensorCommandHandler)
		{
			_sensorCommandHandler->setTemperature(_celsius);
			_sensorCommandHandler->setHumidity(static_cast<uint8_t>(_humidity));
		}

		return TempHumidityCheckMs;
	}
public:
	Dht11SensorHandler(BroadcastManager* broadcastManager, SensorCommandHandler* sensorCommandHandler,
		WarningManager* warningManager, uint8_t sensorPin, float humidityOffset, float temperatureOffset, const char* name = "Dht11")
		: BaseSensor(name), BroadcastLoggerSupport(broadcastManager), _sensorCommandHandler(sensorCommandHandler),
		_warningManager(warningManager), _sensorPin(sensorPin), _humidityOffset(humidityOffset),
		_temperatureOffset(temperatureOffset), _humidity(0.0f), _celsius(0.0f)
	{
#if defined(MQTT_SUPPORT)
		snprintf(_slugTemp, sizeof(_slugTemp), "%s_temperature", _safeSlug);
		snprintf(_slugHumidity, sizeof(_slugHumidity), "%s_humidity", _safeSlug);
		snprintf(_slugDewPoint, sizeof(_slugDewPoint), "%s_dew_point", _safeSlug);
		snprintf(_slugComfort, sizeof(_slugComfort), "%s_comfort", _safeSlug);
		snprintf(_slugCondensation, sizeof(_slugCondensation), "%s_condensation_risk", _safeSlug);
		snprintf(_nameTemp, sizeof(_nameTemp), "%s Temperature", _name);
		snprintf(_nameHumidity, sizeof(_nameHumidity), "%s Humidity", _name);
		snprintf(_nameDewPoint, sizeof(_nameDewPoint), "%s Dew Point", _name);
		snprintf(_nameComfort, sizeof(_nameComfort), "%s Comfort", _name);
		snprintf(_nameCondensation, sizeof(_nameCondensation), "%s Condensation Risk", _name);
#endif
	}

	void formatStatusJson(char* buffer, size_t size) override
	{
        // Validate output buffer
		if (!buffer || size == 0)
		{
			return;
		}

		char celsius[8];
		char humidity[8];
		char celsiusOffset[8];
		char humidityOffset[8];
		dtostrf(_celsius, 1, 1, celsius);
		dtostrf(_humidity, 1, 1, humidity);
		dtostrf(_temperatureOffset, 1, 1, celsiusOffset);
		dtostrf(_humidityOffset, 1, 1, humidityOffset);

		double dewPt = Environment::dewPoint(_celsius, _humidity);
		char dewPointStr[8];
		dtostrf(dewPt, 1, 1, dewPointStr);

		char comfortBuf[24];
		strncpy_P(comfortBuf, Environment::getComfortDescription(_celsius, _humidity, dewPt), sizeof(comfortBuf));
		comfortBuf[sizeof(comfortBuf) - 1] = '\0';

		CondensationRisk risk = Environment::condensationRisk(_celsius, dewPt, false);
		char riskBuf[8];
		strncpy_P(riskBuf, Environment::getCondensationRiskLabel(risk), sizeof(riskBuf));
		riskBuf[sizeof(riskBuf) - 1] = '\0';

		char safeName[64];
		SystemFunctions::sanitizeJsonString(_name, safeName, sizeof(safeName));

		int written = snprintf(buffer, size,
			"\"name\":\"%s\",\"SensorPin\":%u,\"temperature\":%s,\"tempOffset\":%s,\"humidity\":%s,\"humOffset\":%s,\"dew_point\":%s,\"comfort\":\"%s\",\"condensation_risk\":\"%s\"",
			safeName, _sensorPin, celsius, celsiusOffset, humidity, humidityOffset, dewPointStr, comfortBuf, riskBuf);

		if (written < 0 || (size_t)written >= size)
		{
			sendError("Status JSON truncated", _name);
			buffer[size - 1] = '\0';
		}
	}

	SensorIdList getSensorIdType() const override
	{
		return SensorIdList::Dht11Sensor;
	}

	SensorType getSensorType() const override
	{
		return SensorType::Local;
	}

	const char* getSensorCommandId() const override
	{
		return SensorTemperature;
	}

#if defined(MQTT_SUPPORT)
	uint8_t getMqttChannelCount() const override
	{
		return 5;
	}

	MqttSensorChannel getMqttChannel(uint8_t channelIndex) const override
	{
		switch (channelIndex)
		{
			case 0:
				return { _nameTemp, _slugTemp, "temperature", "temperature", "\xc2\xb0""C", false };
			case 1:
				return { _nameHumidity, _slugHumidity, "humidity", "humidity", "%", false };
			case 2:
				return { _nameDewPoint, _slugDewPoint, "humidity", "temperature", "\xc2\xb0""C", false };
			case 3:
				return { _nameComfort, _slugComfort, "humidity", nullptr, nullptr, false };
			case 4:
				return { _nameCondensation, _slugCondensation, "humidity", nullptr, nullptr, false };
			default:
				return { _nameHumidity, _slugHumidity, "humidity", "humidity",    "%", false };
		}
	}

	void getMqttValue(uint8_t channelIndex, char* buffer, size_t size) const override
	{
		switch (channelIndex)
		{
			case 0:
				dtostrf(_celsius, 1, 1, buffer);
				break;
			case 1:
				snprintf(buffer, size, "%u", static_cast<uint8_t>(_humidity));
				break;
			case 2:
			{
				double dewPt = Environment::dewPoint(_celsius, _humidity);
				dtostrf(dewPt, 1, 1, buffer);
				break;
			}
			case 3:
			{
				double dewPt = Environment::dewPoint(_celsius, _humidity);
				strncpy_P(buffer, Environment::getComfortDescription(_celsius, _humidity, dewPt), size);
				buffer[size - 1] = '\0';
				break;
			}
			case 4:
			{
				double dewPt = Environment::dewPoint(_celsius, _humidity);
				CondensationRisk risk = Environment::condensationRisk(_celsius, dewPt, false);
				strncpy_P(buffer, Environment::getCondensationRiskLabel(risk), size);
				buffer[size - 1] = '\0';
				break;
			}
			default:
				if (size > 0) buffer[0] = '\0';
				break;
		}
	}
#endif
};