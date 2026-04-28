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

#include "Local.h"
#include "MessageBus.h"
#if defined(LED_MANAGER)

constexpr auto LedUpdateFrequency = 500;

const long rssiRate[5] = {-40, -50, -60, -70, -80 };
const uint8_t MaxLedRows = 8;
const uint8_t MaxLedColumns = 12;
const uint8_t LedOn = 1;
const uint8_t LedOff = 0;

/*
* X = Used
* 0 = Unused

  { X, X, X, X, X, X, X, X, X, X, X, X },
  { X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, 0, 0, X, X, 0, 0, 0, 0, 0, 0, 0 },
  { X, 0, 0, X, X, 0, 0, 0, 0, 0, 0, 0 },
  { X, 0, 0, X, X, 0, 0, 0, 0, 0, X, X },
  { X, X, 0, X, X, 0, 0, 0, 0, 0, X, X },
  { X, X, 0, X, X, 0, 0, 0, 0, 0, X, X },
  { X, X, X, X, X, 0, 0, 0, 0, 0, X, X }
  
*/
enum class LedSequenceType {
	None,
	Startup,
	Shutdown
};

class LedMatrixManager
{
private:
	MessageBus* _messageBus;
	uint64_t _nextLedUpdate;
	ArduinoLEDMatrix* _matrix;
	uint8_t _ledFrame[MaxLedRows][MaxLedColumns];
	uint8_t _relayStates;
	float _temperature;
	uint64_t _lastTemperatureUpdate;
	float _humididty;
	uint64_t _lastHumidityUpdate;
	LedSequenceType _activeSequence;
	uint8_t _sequenceStep;
	uint64_t _sequenceLastStepTime;
	uint64_t _sequenceDelay;
	bool _sequenceIsOn;
	uint8_t _shutdownTopRow;
	uint8_t _shutdownBottomRow;
	uint8_t _shutdownLeftColumn;
	uint8_t _shutdownRightColumn;
	bool _ledInitialized;

	void updateTemperature(uint64_t currMillis);
	void updateHumidity(uint64_t currMillis);
	void updateLed();
	void processStartupSequence(uint64_t currMillis);
	void processShutdownSequence(uint64_t currMillis);
	void UpdateConnectedState(WifiConnectionState status);
	void UpdateSignalStrength(int16_t strenth);
	void setRelayStatus(uint8_t relayStatus);
	void UpdateWarningIndicators(uint32_t warningMask);
	void SetTemperature(float temperature);
	void SetHumidity(float humidity);

public:
	LedMatrixManager(MessageBus* messageBus);
	~LedMatrixManager();
	void Initialize();
	void ProcessLedMatrix(uint64_t currMillis);
	void UpdateLedFrame(uint8_t state);
	void UpdateRow(uint8_t row, uint8_t state);
	void UpdateColumn(uint8_t column, uint8_t state);
	void StartupSequence();
	void ShutdownSequence();
};
#endif