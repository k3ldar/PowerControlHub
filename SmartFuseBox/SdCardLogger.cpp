#include "SdCardLogger.h"
#include "DateTimeManager.h"
#include "SmartFuseBoxConstants.h"

SdCardLogger::SdCardLogger(MessageBus* messageBus, WarningManager* warningManager)
    : _messageBus(messageBus)
    , _warningManager(warningManager)
    , _initialized(false)
    , _sdCardPresent(false)
    , _bufferHead(0)
    , _bufferTail(0)
    , _bufferCount(0)
    , _currentDay(0)
    , _lastWriteTime(0)
    , _lastFileCheckTime(0)
    , _totalRecordsLogged(0)
    , _recordsDropped(0)
    , _sdCardErrorRaised(false)
{
    memset(_currentFileName, 0, sizeof(_currentFileName));
}

bool SdCardLogger::initialize()
{
    // Subscribe to MessageBus events
    _messageBus->subscribe<TemperatureUpdated>([this](float temp)
        {
        this->onTemperatureUpdated(temp);
    });
    
    _messageBus->subscribe<HumidityUpdated>([this](uint8_t humidity)
        {
        this->onHumidityUpdated(humidity);
    });
    
    _messageBus->subscribe<WaterLevelUpdated>([this](uint16_t level, uint16_t avgLevel)
        {
        this->onWaterLevelUpdated(level, avgLevel);
    });
    
    _messageBus->subscribe<LightSensorUpdated>([this](bool isDayTime)
        {
        this->onLightSensorUpdated(isDayTime);
    });
    
    // Initialize SD card
    if (!initializeSdCard())
    {
        _warningManager->raiseWarning(WarningType::SdCardError);
        _sdCardErrorRaised = true;
        return false;
    }
    
    _initialized = true;
    _sdCardPresent = true;
    
    return true;
}

bool SdCardLogger::initializeSdCard()
{
    // Initialize SPI and SD card
    if (!_sd.begin(SD_CHIP_SELECT_PIN))
    {
        return false;
    }
    
    return true;
}

void SdCardLogger::update(unsigned long now)
{
    if (!_initialized || !_sdCardPresent)
    {
        return;
    }
    
    // Check for date change periodically
    if (now - _lastFileCheckTime >= SD_FILE_CHECK_INTERVAL_MS)
    {
        checkForDateChange(now);
        _lastFileCheckTime = now;
    }
    
    // Write buffered records if enough time has passed
    if (now - _lastWriteTime >= SD_WRITE_INTERVAL_MS)
    {
        if (!isBufferEmpty())
        {
            if (writeRecordsToCard(SD_MAX_WRITES_PER_LOOP))
            {
                _lastWriteTime = now;
                
                // Clear SD card error if it was raised and we can write again
                if (_sdCardErrorRaised)
                {
                    _warningManager->clearWarning(WarningType::SdCardError);
                    _sdCardErrorRaised = false;
                }
            } else {
                // Write failed
                if (!_sdCardErrorRaised)
                {
                    _warningManager->raiseWarning(WarningType::SdCardError);
                    _sdCardErrorRaised = true;
                }
            }
        }
    }
}

bool SdCardLogger::writeRecordsToCard(uint8_t maxRecords)
{
    uint8_t recordsWritten = 0;
    
    // Open or create file if needed
    if (!_currentFile)
    {
        if (!openOrCreateFile(millis()))
        {
            return false;
        }
    }
    
    // Write up to maxRecords from buffer
    while (!isBufferEmpty() && recordsWritten < maxRecords)
    {
        const SensorDataRecord& record = _buffer[_bufferTail];
        
        writeRecordToCsv(record);
        
        // Move tail forward (circular)
        _bufferTail = (_bufferTail + 1) % SD_BUFFER_SIZE;
        _bufferCount--;
        _totalRecordsLogged++;
        recordsWritten++;
    }
    
    // Flush to ensure data is written
    if (recordsWritten > 0)
    {
        _currentFile.flush();
    }
    
    return true;
}

void SdCardLogger::writeRecordToCsv(const SensorDataRecord& record)
{
    if (!_currentFile)
    {
        return;
    }
    
    // Format: Timestamp,DateTime,SensorType,Value1,Value2
    
    // Timestamp (millis)
    _currentFile.print(record.timestamp);
    _currentFile.print(',');
    
    // DateTime (formatted)
    char dateTimeBuf[DateTimeBufferLength];
    DateTimeManager::formatDateTime(dateTimeBuf, sizeof(dateTimeBuf));
    _currentFile.print(dateTimeBuf);
    _currentFile.print(',');
    
    // Sensor type
    _currentFile.print(getSensorTypeName(record.sensorType));
    _currentFile.print(',');
    
    // Value1
    _currentFile.print(record.value1, 2);
    _currentFile.print(',');
    
    // Value2
    _currentFile.print(record.value2, 2);
    _currentFile.println();
}

bool SdCardLogger::openOrCreateFile(unsigned long now)
{
    // Close existing file if open
    if (_currentFile)
    {
        closeCurrentFile();
    }

    // Generate filename based on current date
    updateFileName(now);

    // Check if file exists
    bool fileExists = _sd.exists(_currentFileName);

    // Open file in append mode (O_RDWR | O_CREAT | O_APPEND)
    if (!_currentFile.open(_currentFileName, O_RDWR | O_CREAT | O_AT_END))
    {
        return false;
    }

    // Write CSV header if new file
    if (!fileExists)
    {
        _currentFile.println(F("Timestamp,DateTime,SensorType,Value1,Value2"));
        _currentFile.flush();
    }

    return true;
}

void SdCardLogger::closeCurrentFile()
{
    if (_currentFile)
    {
        _currentFile.flush();
        _currentFile.close();
    }
}

void SdCardLogger::updateFileName(unsigned long now)
{
    uint16_t year = DateTimeManager::getYear();
    uint8_t month = DateTimeManager::getMonth();
    uint8_t day = DateTimeManager::getDay();
    
    // Format: YYYYMMDD.csv
    snprintf(_currentFileName, sizeof(_currentFileName), 
             "%04d%02d%02d.csv", year, month, day);
    
    _currentDay = day;
}

void SdCardLogger::checkForDateChange(unsigned long now)
{
    uint8_t currentDay = DateTimeManager::getDay();
    
    if (currentDay != _currentDay && _currentDay != 0)
    {
        // Date has changed, close current file and open new one
        closeCurrentFile();
        openOrCreateFile(now);
    }
}

void SdCardLogger::addRecordToBuffer(const SensorDataRecord& record)
{
    if (isBufferFull())
    {
        // Buffer is full, drop oldest record (overwrite tail)
        _recordsDropped++;
        _bufferTail = (_bufferTail + 1) % SD_BUFFER_SIZE;
        _bufferCount--;
    }
    
    // Add new record at head
    _buffer[_bufferHead] = record;
    _bufferHead = (_bufferHead + 1) % SD_BUFFER_SIZE;
    _bufferCount++;
}

bool SdCardLogger::isBufferFull() const {
    return _bufferCount >= SD_BUFFER_SIZE;
}

bool SdCardLogger::isBufferEmpty() const {
    return _bufferCount == 0;
}

const char* SdCardLogger::getSensorTypeName(SensorDataType type) const {
    switch (type)
    {
        case SensorDataType::Temperature:
            return "Temperature";
        case SensorDataType::Humidity:
            return "Humidity";
        case SensorDataType::WaterLevel:
            return "WaterLevel";
        case SensorDataType::LightLevel:
            return "LightLevel";
        default:
            return "Unknown";
    }
}

void SdCardLogger::flush()
{
    if ( !_initialized)
    {
        return;
    }
    
    // Write all buffered records (blocking)
    while (!isBufferEmpty())
    {
        writeRecordsToCard(SD_MAX_WRITES_PER_LOOP);
    }
    
    closeCurrentFile();
}

// MessageBus callback implementations
void SdCardLogger::onTemperatureUpdated(float temperature)
{
    SensorDataRecord record(millis(), SensorDataType::Temperature, temperature);
    addRecordToBuffer(record);
}

void SdCardLogger::onHumidityUpdated(uint8_t humidity)
{
    SensorDataRecord record(millis(), SensorDataType::Humidity, static_cast<float>(humidity));
    addRecordToBuffer(record);
}

void SdCardLogger::onWaterLevelUpdated(uint16_t waterLevel, uint16_t averageWaterLevel)
{
    SensorDataRecord record(millis(), SensorDataType::WaterLevel, 
                           static_cast<float>(waterLevel), 
                           static_cast<float>(averageWaterLevel));
    addRecordToBuffer(record);
}

void SdCardLogger::onLightSensorUpdated(bool isDayTime)
{
    SensorDataRecord record(millis(), SensorDataType::LightLevel, 
                           isDayTime ? 1.0f : 0.0f);
    addRecordToBuffer(record);
}
