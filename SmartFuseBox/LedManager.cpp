#include "LedManager.h"

LedManager::LedManager(WifiController* wifiController)
	: _wifiController(wifiController),
	_nextLedUpdate(0),
	_matrix(nullptr),
	_temperature(0),
	_lastTemperatureUpdate(0),
	_humididty(0),
	_lastHumidityUpdate(0)
{
	_matrix = new ArduinoLEDMatrix();
	UpdateLedFrame(LedOff);
}

LedManager::~LedManager()
{
	delete _matrix;
}

void LedManager::Initialize()
{
	_matrix->begin();
}

void LedManager::UpdateConnectedState(WifiConnectionState status)
{
	_ledFrame[6][1] = LedOff;
	_ledFrame[7][1] = LedOff;
	
    if (status == WL_CONNECTED)
    {
		_ledFrame[6][1] = LedOn;
		_ledFrame[7][1] = LedOn;
    }
    else if (status == WL_CONNECTING)
    {
		_ledFrame[7][1] = LedOn;
    }
}

void LedManager::UpdateSignalStrength(int16_t strenth)
{
    _ledFrame[MaxLedRows -1][0] = LedOn;

    for (int i = 2; i < MaxLedRows -1; i++)
        _ledFrame[i][0] = strenth >= rssiRate[i -2] ? LedOn : LedOff;

    if (strenth == 0)
    {
        _ledFrame[2][0] = LedOff;
        _ledFrame[4][0] = LedOff;
        _ledFrame[6][0] = LedOff;
    }
}

void LedManager::ProcessLedMatrix(uint64_t currMillis)
{
	bool hasDelay = false;
	
	if (_wifiController)
	{

		bool canUpdate = currMillis > _nextLedUpdate;
		
		if (canUpdate)
		{
			UpdateSignalStrength(_wifiController->getServer()->getSignalStrength());
			UpdateConnectedState(_wifiController->getServer()->getConnectionState());
			_nextLedUpdate = currMillis + 300;
		}
	}
		
	updateTemperature(currMillis);
	updateHumidity(currMillis);

	updateLed(0);
    
	if (hasDelay)
        delay(50);
}

void LedManager::UpdateLedFrame(uint8_t state)
{
	for (int i = 0; i < MaxLedRows; i++)
	{
		for (int j = 0; j < MaxLedColumns; j++)
		{
			_ledFrame[i][j] = state;
		}
	}
}

void LedManager::UpdateRow(uint8_t row, uint8_t state)
{
	if (row > MaxLedRows -1)
		return;
	
	for (int i = 0; i < MaxLedColumns; i++)
	{
		_ledFrame[row][i] = state;
	}
}

void LedManager::UpdateColumn(uint8_t column, uint8_t state)
{
	if (column > MaxLedColumns -1)
		return;
	
	for (int i = 0; i < MaxLedRows; i++)
	{
		_ledFrame[i][column] = state;
	}
}
	
void LedManager::StartupSequence()
{
	for (int i = 0; i < MaxLedRows; i++)
	{
		if (i % 2)
		{
			for (int j = MaxLedColumns -1; j >= 0; j--)
			{
				_ledFrame[i][j] = LedOn;
				updateLed(30);
			}
		}
		else
		{
			for (int j = 0; j < MaxLedColumns; j++)
			{
				_ledFrame[i][j] = LedOn;
				updateLed(30);
			}
		}
	}
	
	bool isOn = true;
	
	for (int i = 0; i < 10; i++)
	{
		UpdateLedFrame(isOn ? LedOn : LedOff);
		isOn = !isOn;
		updateLed(200);
	}
}

void LedManager::ShutdownSequence()
{
	UpdateLedFrame(LedOff);
	updateLed(100);
	int topRow = 0;
	int bottomRow = 7;
	int leftColumn = 0;
	int rightColumn = 11;
	
	for (int i = 0; i < 5; i++)
	{
		UpdateRow(topRow, LedOn);
		UpdateRow(bottomRow, LedOn);
		UpdateColumn(leftColumn, LedOn);
		UpdateColumn(rightColumn, LedOn);
		updateLed(400);
		
		topRow++;
		bottomRow--;
		leftColumn++;
		rightColumn--;
	}
	
	UpdateLedFrame(LedOff);
	updateLed(500);
	
	_ledFrame[2][3] = LedOn;
	_ledFrame[2][4] = LedOn;
	_ledFrame[2][7] = LedOn;
	_ledFrame[2][8] = LedOn;
	updateLed(800);
}

void LedManager::SetTemperature(float temperature)
{
	_temperature = temperature;
}

void LedManager::SetHumidity(float humidity)
{
	_humididty = humidity;
}

// Private methods

void LedManager::updateLed(int delayMs)
{
	_matrix->renderBitmap(_ledFrame, MaxLedRows, MaxLedColumns);
	delay(delayMs);
}

void LedManager::updateTemperature(uint64_t currMillis)
{
	if (currMillis < _lastTemperatureUpdate)
		return;
	
	_ledFrame[7][4] = _temperature > 0 && _temperature < 0 ? LedOn : LedOff;
	_ledFrame[6][4] = _temperature > 0 ? LedOn : LedOff;
	_ledFrame[5][4] = _temperature > 8 ? LedOn : LedOff;
	_ledFrame[4][4] = _temperature > 16 ? LedOn : LedOff;
	_ledFrame[3][4] = _temperature > 24 ? LedOn : LedOff;
	_ledFrame[2][4] = _temperature > 32 ? LedOn : LedOff;
	
	_lastTemperatureUpdate = currMillis + LED_UPDATE_FREQUENCY;
}

void LedManager::updateHumidity(uint64_t currMillis)
{
	if (currMillis < _lastHumidityUpdate)
		return;
	
	_ledFrame[7][5] = _humididty > 40 ? LedOn : LedOff;
	_ledFrame[6][5] = _humididty > 50 ? LedOn : LedOff;
	_ledFrame[5][5] = _humididty > 60 ? LedOn : LedOff;
	_ledFrame[4][5] = _humididty > 70 ? LedOn : LedOff;
	_ledFrame[3][5] = _humididty > 80 ? LedOn : LedOff;
	_ledFrame[2][5] = _humididty > 90 ? LedOn : LedOff;
	
	_lastHumidityUpdate = currMillis + LED_UPDATE_FREQUENCY;
}
