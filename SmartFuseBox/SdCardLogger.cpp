#include "SdCardLogger.h"
#include "DateTimeManager.h"
#include "SmartFuseBoxConstants.h"
#include <SPI.h>

SdCardLogger::SdCardLogger(SensorCommandHandler* sensorHandler, WarningManager* warningManager, uint8_t csPin)
    : _sensorHandler(sensorHandler),
      _warningManager(warningManager),
      _csPin(csPin),
      _initialized(false),
      _sdCardPresent(false),
      _bufferHead(0),
      _bufferTail(0),
      _bufferCount(0),
      _currentDay(0),
      _lastWriteTime(0),
      _lastFileCheckTime(0),
      _lastCardPresenceCheck(0),
      _totalRecordsLogged(0),
      _recordsDropped(0),
      _sdCardErrorRaised(false),
      _sdCardMissingRaised(false)
{
    memset(_currentFileName, 0, sizeof(_currentFileName));
}

bool SdCardLogger::initialize()
{
    if (!initializeSdCard())
    {
        // Check if it's a card missing error or other error
        if (isCardMissingError())
        {
            _warningManager->raiseWarning(WarningType::SdCardMissing);
            _sdCardMissingRaised = true;
        }
        else
        {
            _warningManager->raiseWarning(WarningType::SdCardError);
            _sdCardErrorRaised = true;
        }
        return false;
    }
    
    _initialized = true;
    _sdCardPresent = true;
    
    return true;
}

bool SdCardLogger::initializeSdCard()
{
    // Explicitly set CS pin as output (best practice, though SdFat usually handles this)
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH); // Deselect card initially
    
    // Explicitly initialize SPI before SD card
    // SPI.begin() automatically configures:
    //   - MOSI (D11) as OUTPUT
    //   - MISO (D12) as INPUT  
    //   - SCK (D13) as OUTPUT
    SPI.begin();
    
    // Small delay to allow SPI to stabilize
    delay(10);
    
    Serial.println(F("Initializing SD card..."));
    Serial.print(F("CS Pin: "));
    Serial.println(_csPin);
    
    // Initialize SD card with explicit CS pin and slower speed for reliability
    // SD_SCK_MHZ(4) = 4MHz (try 1, 2, or 4 if having issues)
    if (!_sd.begin(_csPin, SD_SCK_MHZ(4)))
    {
        Serial.println(F("SD card initialization FAILED"));
        Serial.print(F("Error code: 0x"));
        Serial.println(_sd.card()->errorCode(), HEX);
        Serial.print(F("Error data: 0x"));
        Serial.println(_sd.card()->errorData(), HEX);
        return false;
    }
    
    Serial.println(F("SD card initialized successfully!"));
    
    // Print card info for debugging
    uint32_t cardSize = _sd.card()->sectorCount() * 512UL / (1024 * 1024);
    Serial.print(F("Card size: "));
    Serial.print(cardSize);
    Serial.println(F(" MB"));
    
    return true;
}

void SdCardLogger::update(unsigned long now)
{
    bool checkingPresence = false;

    // Periodically check for card presence changes (insertion/removal)
    if (now - _lastCardPresenceCheck >= SD_CARD_PRESENCE_CHECK_MS)
    {
        checkingPresence = true;
        Serial.println("Checking card presence");
        checkCardPresence(now);
        _lastCardPresenceCheck = now;
    }

    if (!_initialized || !_sdCardPresent)
    {
        if (checkingPresence)
        {
            Serial.println("SD not initialized/present");
        }

        return;
    }

    // Check for date change periodically
    if (now - _lastFileCheckTime >= SD_FILE_CHECK_INTERVAL_MS)
    {
        checkForDateChange(now);
        _lastFileCheckTime = now;
    }

    // Capture sensor snapshot and write to SD every second
    if (now - _lastWriteTime >= SD_WRITE_INTERVAL_MS)
    {
        Serial.println("Capturing sensor snapshot");
        
        // Capture current sensor state
        captureSensorSnapshot();
        
        // Write buffered records
        if (!isBufferEmpty())
        {
            Serial.println("Writing logs to SD");
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
        else
        {
            _lastWriteTime = now;
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
        const SensorSnapshot& snapshot = _buffer[_bufferTail];
        
        writeSnapshotToCsv(snapshot);
        
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

void SdCardLogger::writeSnapshotToCsv(const SensorSnapshot& snapshot)
{
    if (!_currentFile)
    {
        return;
    }
    
    // Format: Timestamp,Temp,Humidity,Bearing,CompassTemp,Speed,WaterLevel,WaterPump,GPSLat,GPSLon,Altitude,GPSCourse,GPSSats,GPSDistance,Warnings,Horn
    
    // DateTime (formatted)
    char dateTimeBuf[DateTimeBufferLength];
    DateTimeManager::formatDateTime(dateTimeBuf, sizeof(dateTimeBuf));
    _currentFile.print(dateTimeBuf);
    _currentFile.print(',');
    
    // Temperature
    if (isnan(snapshot.temperature))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.temperature, 1);
    _currentFile.print(',');
    
    // Humidity
    _currentFile.print(snapshot.humidity);
    _currentFile.print(',');
    
    // Bearing
    if (isnan(snapshot.bearing))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.bearing, 1);
    _currentFile.print(',');
    
    // CompassTemp
    if (isnan(snapshot.compassTemp))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.compassTemp, 1);
    _currentFile.print(',');
    
    // Speed
    _currentFile.print(snapshot.speed);
    _currentFile.print(',');
    
    // WaterLevel
    _currentFile.print(snapshot.waterLevel);
    _currentFile.print(',');
    
    // WaterPump (0 or 1)
    _currentFile.print(snapshot.waterPumpActive ? '1' : '0');
    _currentFile.print(',');
    
    // GPSLat
    if (isnan(snapshot.gpsLat))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.gpsLat, 6);
    _currentFile.print(',');
    
    // GPSLon
    if (isnan(snapshot.gpsLon))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.gpsLon, 6);
    _currentFile.print(',');
    
    // Altitude
    if (isnan(snapshot.altitude))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.altitude, 1);
    _currentFile.print(',');
    
    // GPSCourse
    if (isnan(snapshot.gpsCourse))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.gpsCourse, 1);
    _currentFile.print(',');
    
    // GPSSats
    _currentFile.print(snapshot.gpsSats);
    _currentFile.print(',');
    
    // GPSDistance
    if (isnan(snapshot.gpsDistance))
        _currentFile.print('?');
    else
        _currentFile.print(snapshot.gpsDistance, 2);
    _currentFile.print(',');
    
    // Warnings (hex format with leading zeros to 8 digits)
    _currentFile.print(F("0x"));
    char hexBuf[9];
    snprintf(hexBuf, sizeof(hexBuf), "%08lX", (unsigned long)snapshot.warnings);
    _currentFile.print(hexBuf);
    _currentFile.print(',');
    
    // Horn (0 or 1)
    _currentFile.print(snapshot.hornActive ? '1' : '0');
    
    _currentFile.println();
}

bool SdCardLogger::openOrCreateFile(unsigned long now)
{
    Serial.println("SD open or create file");
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
        Serial.println("Writing SD header");
        _currentFile.println(F("Timestamp,Temp,Humidity,Bearing,CompassTemp,Speed,WaterLevel,WaterPump,GPSLat,GPSLon,Altitude,GPSCourse,GPSSats,GPSDistance,Warnings,Horn"));
        _currentFile.flush();
    }

    return true;
}

void SdCardLogger::closeCurrentFile()
{
    if (_currentFile)
    {
        Serial.println("SD closing file");
        _currentFile.flush();
        _currentFile.close();
    }
}

void SdCardLogger::updateFileName(unsigned long now)
{
	(void)now; // Unused parameter, but could be used for future enhancements
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

void SdCardLogger::captureSensorSnapshot()
{
    if (!_sensorHandler)
    {
        return;
    }
    
    SensorSnapshot snapshot;
    
    // Capture all sensor values
    snapshot.temperature = _sensorHandler->getTemperature();
    snapshot.humidity = _sensorHandler->getHumidity();
    snapshot.bearing = _sensorHandler->getBearing();
    snapshot.compassTemp = _sensorHandler->getCompassTemperature();
    snapshot.speed = _sensorHandler->getSpeed();
    snapshot.waterLevel = _sensorHandler->getWaterLevel();
    snapshot.waterPumpActive = _sensorHandler->getWaterPumpActive();
    snapshot.gpsLat = _sensorHandler->getGpsLatitude();
    snapshot.gpsLon = _sensorHandler->getGpsLongitude();
    snapshot.altitude = _sensorHandler->getGpsAltitude();
    snapshot.gpsCourse = _sensorHandler->getGpsCourse();
    snapshot.gpsSats = _sensorHandler->getGpsSatellites();
    snapshot.gpsDistance = _sensorHandler->getGpsDistance();
    snapshot.warnings = _warningManager->getActiveWarningsMask();
    snapshot.hornActive = _sensorHandler->getHornActive();
    
    addSnapshotToBuffer(snapshot);
}

void SdCardLogger::addSnapshotToBuffer(const SensorSnapshot& snapshot)
{
    if (isBufferFull())
    {
        Serial.println("SD buffer full");
        // Buffer is full, drop oldest record (overwrite tail)
        _recordsDropped++;
        _bufferTail = (_bufferTail + 1) % SD_BUFFER_SIZE;
        _bufferCount--;
    }
    
    Serial.println("Add to SD buffer");
    // Add new record at head
    _buffer[_bufferHead] = snapshot;
    _bufferHead = (_bufferHead + 1) % SD_BUFFER_SIZE;
    _bufferCount++;
}

bool SdCardLogger::isBufferFull() const
{
    return _bufferCount >= SD_BUFFER_SIZE;
}

bool SdCardLogger::isBufferEmpty() const
{
    return _bufferCount == 0;
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

void SdCardLogger::checkCardPresence(unsigned long now)
{
	(void)now; // Unused parameter, but could be used for future enhancements

    // Try to re-initialize SD card with slower speed for reliability
    bool cardPresent = _sd.begin(_csPin, SD_SCK_MHZ(4));

    // Card state changed from missing to present
    if (cardPresent && !_sdCardPresent)
    {
        Serial.println("Card Present");
        _sdCardPresent = true;
        _initialized = true;

        // Clear any SD card warnings
        if (_sdCardMissingRaised)
        {
            _warningManager->clearWarning(WarningType::SdCardMissing);
            _sdCardMissingRaised = false;
        }

        if (_sdCardErrorRaised)
        {
            _warningManager->clearWarning(WarningType::SdCardError);
            _sdCardErrorRaised = false;
        }
    }
    // Card state changed from present to missing
    else if (!cardPresent && _sdCardPresent)
    {
        Serial.println("Card not present");
        _sdCardPresent = false;
        _initialized = false;

        // Close any open files
        closeCurrentFile();

        // Determine if card is missing or there's an error
        if (isCardMissingError())
        {
            // Clear error warning if it was raised
            if (_sdCardErrorRaised)
            {
                _warningManager->clearWarning(WarningType::SdCardError);
                _sdCardErrorRaised = false;
            }

            // Raise missing warning if not already raised
            if (!_sdCardMissingRaised)
            {
                _warningManager->raiseWarning(WarningType::SdCardMissing);
                _sdCardMissingRaised = true;
            }
        }
        else
        {
            Serial.println("Card Missing or other error");
            // Clear missing warning if it was raised
            if (_sdCardMissingRaised)
            {
                _warningManager->clearWarning(WarningType::SdCardMissing);
                _sdCardMissingRaised = false;
            }

            // Raise error warning if not already raised
            if (!_sdCardErrorRaised)
            {
                _warningManager->raiseWarning(WarningType::SdCardError);
                _sdCardErrorRaised = true;
            }
        }
    }
}

bool SdCardLogger::isCardMissingError()
{
    // Check the SD card error code to determine if card is missing
    // Common error codes for missing card:
    // - SD_CARD_ERROR_CMD0: Card not responding to CMD0 (GO_IDLE_STATE)
    // - SD_CARD_ERROR_ACMD41: Card not responding to ACMD41 (initialization)
    // - 0xFF or 0x01: Card not present (no response on SPI bus)

    uint8_t errorCode = _sd.card()->errorCode();

    Serial.print("Card error code: ");
    Serial.println(errorCode);

    // Error codes that typically indicate missing card:
    // 0x01 = SD_CARD_ERROR_CMD0 (card not present/responding)
    // 0x02 = SD_CARD_ERROR_CMD8 (card not responding to voltage check)
    // 0xFF = No card or no response
    return (errorCode == 0x01 || errorCode == 0x02 || errorCode == 0xFF);
}
