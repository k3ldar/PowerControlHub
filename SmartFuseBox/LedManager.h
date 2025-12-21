#pragma once

#include <Arduino_LED_Matrix.h>

#define LED_UPDATE_FREQUENCY 500

const long rssiRate[5] = {-70, -80, -90, -100, -110 };
const int MaxLedRows = 8;
const int MaxLedColumns = 12;
const uint8_t LedOn = 1;
const uint8_t LedOff = 0;

/*

  { X, X, X, X, X, X, X, X, X, X, X, X },
  { X, X, X, X, X, X, X, X, X, X, X, X },
  { X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { X, X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  
*/
class LedManager
{
private:
	WifiController* _wifiController;
	unsigned long _nextLedUpdate;
	ArduinoLEDMatrix* _matrix;
	uint8_t _ledFrame[MaxLedRows][MaxLedColumns];
	float _temperature;
	uint64_t _lastTemperatureUpdate;
	float _humididty;
	uint64_t _lastHumidityUpdate;

	void updateTemperature(uint64_t currMillis);
	void updateHumidity(uint64_t currMillis);
	void updateLed(int delayMs);
public:
	LedManager(WifiController* wifiController);
	~LedManager();
	void Initialize();
	void ProcessLedMatrix(uint64_t currMillis);
	void UpdateSignalStrength(int16_t strenth);
	void UpdateConnectedState(WifiConnectionState status);
	void UpdateLedFrame(uint8_t state);
	void UpdateRow(uint8_t row, uint8_t state);
	void UpdateColumn(uint8_t column, uint8_t state);
	void StartupSequence();
	void ShutdownSequence();
	
	void SetTemperature(float temperature);
	void SetHumidity(float humidity);
};
