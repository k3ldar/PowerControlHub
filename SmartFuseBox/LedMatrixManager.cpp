#include "LedMatrixManager.h"
#include "SmartFuseBoxConstants.h"

LedMatrixManager::LedMatrixManager(MessageBus* messageBus)
	: _messageBus(messageBus),
	_nextLedUpdate(0),
	_matrix(nullptr),
	_temperature(0),
	_lastTemperatureUpdate(0),
	_humididty(0),
	_lastHumidityUpdate(0),
	_activeSequence(LedSequenceType::None),
	_sequenceStep(0),
	_sequenceLastStepTime(0),
	_sequenceDelay(0),
	_sequenceIsOn(false),
	_ledInitialized(false)
{
	if (messageBus)
	{
		messageBus->subscribe<WarningChanged>([this](uint32_t mask) {
			this->UpdateWarningIndicators(mask);
		});
		messageBus->subscribe<WifiConnectionStateChanged>([this](WifiConnectionState status) {
			this->UpdateConnectedState(status);
		});
		messageBus->subscribe<WifiSignalStrengthChanged>([this](uint16_t strength) {
			this->UpdateSignalStrength(strength);
		});
		messageBus->subscribe<RelayStatusChanged>([this](uint8_t status) {
			this->setRelayStatus(status);
		});
		messageBus->subscribe<TemperatureUpdated>([this](float newTemp) {
			this->SetTemperature(newTemp);
		});
		messageBus->subscribe<HumidityUpdated>([this](float newHumidity) {
			this->SetHumidity(newHumidity);
		});
		messageBus->subscribe<WifiServerProcessingRequestChanged>([this](const char* method, const char* path, const char* query, bool isProcessing) {
			(void)method;
			(void)path;
			(void)query;
			this->_ledFrame[7][2] = isProcessing ? LedOn : LedOff;
			this->updateLed();
		});
	}
}

LedMatrixManager::~LedMatrixManager()
{
	delete _matrix;
	_ledInitialized = false;
}

void LedMatrixManager::Initialize()
{
	if (_ledInitialized)
		return;

	_matrix = new ArduinoLEDMatrix();
	_matrix->begin();

	UpdateLedFrame(LedOff);
	_ledInitialized = true;
}

void LedMatrixManager::UpdateConnectedState(WifiConnectionState status)
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

void LedMatrixManager::UpdateSignalStrength(int16_t strength)
{
    _ledFrame[MaxLedRows -1][0] = LedOn;

    for (int i = 2; i < MaxLedRows -1; i++)
        _ledFrame[i][0] = strength >= rssiRate[i -2] ? LedOn : LedOff;

    if (strength == 0)
    {
        _ledFrame[2][0] = LedOff;
        _ledFrame[4][0] = LedOff;
        _ledFrame[6][0] = LedOff;
    }
}

void LedMatrixManager::ProcessLedMatrix(unsigned long currMillis)
{
	if (_activeSequence == LedSequenceType::Startup)
	{
		processStartupSequence(currMillis);
		return;
	}
	else if (_activeSequence == LedSequenceType::Shutdown)
	{
		processShutdownSequence(currMillis);
		return;
	}

	updateTemperature(currMillis);
	updateHumidity(currMillis);
	updateLed();
}

void LedMatrixManager::UpdateLedFrame(uint8_t state)
{
	for (int i = 0; i < MaxLedRows; i++)
	{
		for (int j = 0; j < MaxLedColumns; j++)
		{
			_ledFrame[i][j] = state;
		}
	}
}

void LedMatrixManager::UpdateRow(uint8_t row, uint8_t state)
{
	if (row > MaxLedRows -1)
		return;
	
	for (int i = 0; i < MaxLedColumns; i++)
	{
		_ledFrame[row][i] = state;
	}
}

void LedMatrixManager::UpdateColumn(uint8_t column, uint8_t state)
{
	if (column > MaxLedColumns -1)
		return;
	
	for (int i = 0; i < MaxLedRows; i++)
	{
		_ledFrame[i][column] = state;
	}
}
	
void LedMatrixManager::StartupSequence()
{
	_activeSequence = LedSequenceType::Startup;
	_sequenceStep = 0;
	_sequenceLastStepTime = millis();
	_sequenceDelay = 30;
	_sequenceIsOn = true;
}

void LedMatrixManager::processStartupSequence(unsigned long currMillis)
{
	if (_sequenceStep < MaxLedRows * MaxLedColumns)
	{
		if (currMillis - _sequenceLastStepTime >= _sequenceDelay)
		{
			int i = _sequenceStep / MaxLedColumns;
			int j = (i % 2) ? (MaxLedColumns - 1 - (_sequenceStep % MaxLedColumns)) : (_sequenceStep % MaxLedColumns);
			_ledFrame[i][j] = LedOn;
			updateLed();
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	}
	else if (_sequenceStep < MaxLedRows * MaxLedColumns + 10)
	{
		if (currMillis - _sequenceLastStepTime >= 200)
		{
			UpdateLedFrame(_sequenceIsOn ? LedOn : LedOff);
			updateLed();
			_sequenceIsOn = !_sequenceIsOn;
			_sequenceStep++;
			_sequenceLastStepTime = currMillis;
		}
	}
	else
	{
		_activeSequence = LedSequenceType::None;
	}
}

// Non-blocking Shutdown Sequence
void LedMatrixManager::ShutdownSequence()
{
    _activeSequence = LedSequenceType::Shutdown;
    _sequenceStep = 0;
    _sequenceLastStepTime = millis();
    _sequenceDelay = 400;
    _shutdownTopRow = 0;
    _shutdownBottomRow = MaxLedRows - 1;
    _shutdownLeftColumn = 0;
    _shutdownRightColumn = MaxLedColumns - 1;
}

void LedMatrixManager::processShutdownSequence(unsigned long currMillis)
{
    if (_sequenceStep == 0) {
        UpdateLedFrame(LedOff);
        updateLed();
        _sequenceLastStepTime = currMillis;
        _sequenceStep++;
    } else if (_sequenceStep <= 5) {
        if (currMillis - _sequenceLastStepTime >= _sequenceDelay) {
            UpdateRow(_shutdownTopRow, LedOn);
            UpdateRow(_shutdownBottomRow, LedOn);
            UpdateColumn(_shutdownLeftColumn, LedOn);
            UpdateColumn(_shutdownRightColumn, LedOn);
            updateLed();
            _shutdownTopRow++; _shutdownBottomRow--;
            _shutdownLeftColumn++; _shutdownRightColumn--;
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
    }
}

void LedMatrixManager::SetTemperature(float temperature)
{
	_temperature = temperature;
}

void LedMatrixManager::SetHumidity(float humidity)
{
	_humididty = humidity;
}

// Private methods

void LedMatrixManager::updateLed()
{
	if (!_ledInitialized)
	{
		return;
	}

	if (_matrix && _ledInitialized)
	{
		_matrix->renderBitmap(_ledFrame, MaxLedRows, MaxLedColumns);
	}
}

void LedMatrixManager::updateTemperature(unsigned long currMillis)
{
	if (currMillis < _lastTemperatureUpdate)
		return;
	
	_ledFrame[7][4] = _temperature > 0 && _temperature < 0 ? LedOn : LedOff;
	_ledFrame[6][4] = _temperature > 0 ? LedOn : LedOff;
	_ledFrame[5][4] = _temperature > 8 ? LedOn : LedOff;
	_ledFrame[4][4] = _temperature > 16 ? LedOn : LedOff;
	_ledFrame[3][4] = _temperature > 24 ? LedOn : LedOff;
	_ledFrame[2][4] = _temperature > 32 ? LedOn : LedOff;
	
	_lastTemperatureUpdate = currMillis + LedUpdateFrequency;
}

void LedMatrixManager::updateHumidity(unsigned long currMillis)
{
	if (currMillis < _lastHumidityUpdate)
		return;
	
	_ledFrame[7][5] = _humididty > 40 ? LedOn : LedOff;
	_ledFrame[6][5] = _humididty > 50 ? LedOn : LedOff;
	_ledFrame[5][5] = _humididty > 60 ? LedOn : LedOff;
	_ledFrame[4][5] = _humididty > 70 ? LedOn : LedOff;
	_ledFrame[3][5] = _humididty > 80 ? LedOn : LedOff;
	_ledFrame[2][5] = _humididty > 90 ? LedOn : LedOff;
	
	_lastHumidityUpdate = currMillis + LedUpdateFrequency;
}

void LedMatrixManager::setRelayStatus(uint8_t relayStatus)
{
	_relayStates = relayStatus;

	// Update the LED matrix for each relay
	for (uint8_t relay = 0; relay < TotalRelays; ++relay)
	{
		uint8_t row = relay;
		uint8_t col = relay < 4 ? 10 : 11;

		if (col == 10)
			row += 4;

		// Check if the corresponding bit is set (relay is ON)
		bool isOn = (relayStatus & (1 << relay)) != 0;
		_ledFrame[row][col] = isOn ? LedOn : LedOff;
	}

	updateLed();
}

void LedMatrixManager::UpdateWarningIndicators(uint32_t warningMask)
{
	// Start from bit 2 (skip DefaultConfigurationFuseBox and DefaultConfigurationControlPanel)
	const uint8_t firstWarningBit = 2;
	uint8_t col = 0;

	for (uint8_t bit = firstWarningBit; bit < 32 && col < MaxLedColumns; ++bit, ++col)
	{
		bool isActive = (warningMask & (1UL << bit)) != 0;
		_ledFrame[1][col] = isActive ? LedOn : LedOff;
	}

	// Clear any remaining LEDs in the row if there are fewer warnings than columns
	for (; col < MaxLedColumns; ++col)
	{
		_ledFrame[1][col] = LedOff;
	}

	updateLed();
}