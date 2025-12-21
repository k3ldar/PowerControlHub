#include "LedManager.h"

LedManager::LedManager(WifiController* wifiController)
	: _wifiController(wifiController),
	_nextLedUpdate(0),
	_matrix(nullptr),
	_temperature(0),
	_lastTemperatureUpdate(0),
	_humididty(0),
	_lastHumidityUpdate(0),
	_activeSequence(LedSequenceType::None),
	_sequenceStep(0),
	_sequenceLastStepTime(0),
	_sequenceDelay(0)
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
	
    if (status == WifiConnectionState::Connected)
    {
		_ledFrame[6][1] = LedOn;
		_ledFrame[7][1] = LedOn;
    }
    else if (status == WifiConnectionState::Connecting)
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
	if (_activeSequence == LedSequenceType::Startup) {
		processStartupSequence(currMillis);
		return;
	}
	if (_activeSequence == LedSequenceType::Shutdown) {
		processShutdownSequence(currMillis);
		return;
	}

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

	updateLed();
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
	
// Non-blocking Startup Sequence
void LedManager::StartupSequence()
{
	_activeSequence = LedSequenceType::Startup;
	_sequenceStep = 0;
	_sequenceLastStepTime = millis();
	_sequenceDelay = 30;
}

void LedManager::processStartupSequence(uint64_t currMillis)
{
	static bool isOn = true;
	if (_sequenceStep < MaxLedRows * MaxLedColumns) {
		if (currMillis - _sequenceLastStepTime >= _sequenceDelay) {
			int i = _sequenceStep / MaxLedColumns;
			int j = (i % 2) ? (MaxLedColumns - 1 - (_sequenceStep % MaxLedColumns)) : (_sequenceStep % MaxLedColumns);
			_ledFrame[i][j] = LedOn;
			updateLed();
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	} else if (_sequenceStep < MaxLedRows * MaxLedColumns + 10) {
		if (currMillis - _sequenceLastStepTime >= 200) {
			UpdateLedFrame(isOn ? LedOn : LedOff);
			updateLed();
			isOn = !isOn;
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	} else {
		_activeSequence = LedSequenceType::None;
	}
}

// Non-blocking Shutdown Sequence
void LedManager::ShutdownSequence()
{
	_activeSequence = LedSequenceType::Shutdown;
	_sequenceStep = 0;
	_sequenceLastStepTime = millis();
	_sequenceDelay = 400;
}

void LedManager::processShutdownSequence(uint64_t currMillis)
{
	static int topRow = 0, bottomRow = 7, leftColumn = 0, rightColumn = 11;
	if (_sequenceStep == 0) {
		UpdateLedFrame(LedOff);
		updateLed();
		_sequenceLastStepTime = currMillis;
		_sequenceStep++;
	} else if (_sequenceStep <= 5) {
		if (currMillis - _sequenceLastStepTime >= _sequenceDelay) {
			UpdateRow(topRow, LedOn);
			UpdateRow(bottomRow, LedOn);
			UpdateColumn(leftColumn, LedOn);
			UpdateColumn(rightColumn, LedOn);
			updateLed();
			topRow++; bottomRow--; leftColumn++; rightColumn--;
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	} else if (_sequenceStep == 6) {
		if (currMillis - _sequenceLastStepTime >= 500) {
			UpdateLedFrame(LedOff);
			updateLed();
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	} else if (_sequenceStep == 7) {
		if (currMillis - _sequenceLastStepTime >= 800) {
			_ledFrame[2][3] = LedOn;
			_ledFrame[2][4] = LedOn;
			_ledFrame[2][7] = LedOn;
			_ledFrame[2][8] = LedOn;
			updateLed();
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	} else {
		_activeSequence = LedSequenceType::None;
		topRow = 0; bottomRow = 7; leftColumn = 0; rightColumn = 11;
	}
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

void LedManager::updateLed()
{
	_matrix->renderBitmap(_ledFrame, MaxLedRows, MaxLedColumns);
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
