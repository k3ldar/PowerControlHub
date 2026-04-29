/*
 * PowerControlHub
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
#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include "SdCardLogger.h"
#include "DateTimeManager.h"
#include "PowerControlHubConstants.h"
#include "MicroSdDriver.h"
#include <SPI.h>

SdCardLogger::SdCardLogger(SensorCommandHandler* sensorHandler, WarningManager* warningManager)
    : _sensorHandler(sensorHandler),
      _warningManager(warningManager),
      _currentFile(nullptr),
      _fileOpen(false),
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
      _sdCardMissingRaised(false),
      _initialLogFileSize(0)
{
    memset(_currentFileName, 0, sizeof(_currentFileName));
}

bool SdCardLogger::initialize()
{
    // SdCardLogger now relies on MicroSdDriver for SD card management
    // Just check if MicroSdDriver is ready
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    if (!sdDriver.isReady())
    {
        // MicroSdDriver will handle its own warnings during initialization
        return false;
    }

    return true;
}

void SdCardLogger::update(uint64_t now)
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    // Periodically check SD card status (using MicroSdDriver state)
    if (now - _lastCardPresenceCheck >= SD_CARD_PRESENCE_CHECK_MS)
    {
        checkSdCardStatus();
        _lastCardPresenceCheck = now;
    }

    if (!sdDriver.isReady())
    {
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
        // Capture current sensor state
        captureSensorSnapshot();

        // Write buffered records
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
            }
            else
            {
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
    if (_currentFile != nullptr && !_currentFile->isOpen())
    {
        _currentFile = nullptr;
        _fileOpen = false;
    }

    uint8_t recordsWritten = 0;

    // Open or create file if needed
    if (_currentFile == nullptr || !_fileOpen)
    {
        if (!openOrCreateFile(SystemFunctions::millis64()))
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
    if (recordsWritten > 0 && _currentFile != nullptr)
    {
        _currentFile->flush();
    }

    return true;
}

void SdCardLogger::writeSnapshotToCsv(const SensorSnapshot& snapshot)
{
    if (_currentFile == nullptr || !_fileOpen)
    {
        return;
    }

    // Format: Timestamp,Temp,Humidity,Bearing,CompassTemp,Speed,WaterLevel,WaterPump,GPSLat,GPSLon,Altitude,GPSCourse,GPSSats,GPSDistance,Warnings,Horn

    // DateTime (formatted)
    char dateTimeBuf[DateTimeBufferLength];
    DateTimeManager::formatDateTime(dateTimeBuf, sizeof(dateTimeBuf));
    _currentFile->print(dateTimeBuf);
    _currentFile->print(',');

    // Temperature
    if (isnan(snapshot.temperature))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.temperature, 1);
    _currentFile->print(',');

    // Humidity
    _currentFile->print(snapshot.humidity);
    _currentFile->print(',');

    // Bearing
    if (isnan(snapshot.bearing))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.bearing, 1);
    _currentFile->print(',');

    // CompassTemp
    if (isnan(snapshot.compassTemp))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.compassTemp, 1);
    _currentFile->print(',');

    // Speed
    _currentFile->print(snapshot.speed);
    _currentFile->print(',');

    // WaterLevel
    _currentFile->print(snapshot.waterLevel);
    _currentFile->print(',');

    // WaterPump (0 or 1)
    _currentFile->print(snapshot.waterPumpActive ? '1' : '0');
    _currentFile->print(',');

    // GPSLat
    if (isnan(snapshot.gpsLat))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.gpsLat, 6);
    _currentFile->print(',');

    // GPSLon
    if (isnan(snapshot.gpsLon))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.gpsLon, 6);
    _currentFile->print(',');

    // Altitude
    if (isnan(snapshot.altitude))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.altitude, 1);
    _currentFile->print(',');

    // GPSCourse
    if (isnan(snapshot.gpsCourse))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.gpsCourse, 1);
    _currentFile->print(',');

    // GPSSats
    _currentFile->print(snapshot.gpsSats);
    _currentFile->print(',');

    // GPSDistance
    if (isnan(snapshot.gpsDistance))
        _currentFile->print('?');
    else
        _currentFile->print(snapshot.gpsDistance, 2);
    _currentFile->print(',');

    // Warnings (hex format with leading zeros to 8 digits)
    _currentFile->print(F("0x"));
    char hexBuf[9];
    snprintf(hexBuf, sizeof(hexBuf), "%08lX", (unsigned long)snapshot.warnings);
    _currentFile->print(hexBuf);
    _currentFile->print(',');

    // Horn (0 or 1)
    _currentFile->print(snapshot.hornActive ? '1' : '0');

    _currentFile->println();
}

bool SdCardLogger::openOrCreateFile(uint64_t now)
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    // Close existing file if open
    if (_currentFile != nullptr && _fileOpen)
    {
        closeCurrentFile();
    }

    // Generate filename based on current date
    updateFileName(now);

    // Check if file exists
    bool fileExists = sdDriver.fileExists(_currentFileName);

    // Open file using MicroSdDriver with Logger handle
    _currentFile = sdDriver.openFile(MicroSdFileHandle::Logger, _currentFileName, O_RDWR | O_CREAT | O_AT_END);

    if (_currentFile == nullptr)
    {
        _fileOpen = false;
        return false;
    }

    _fileOpen = true;

    // Track initial file size for free space calculation
    _initialLogFileSize = _currentFile->fileSize();

    // Write CSV header if new file
    if (!fileExists)
    {
        _currentFile->println(F("Timestamp,Temp,Humidity,Bearing,CompassTemp,Speed,WaterLevel,WaterPump,GPSLat,GPSLon,Altitude,GPSCourse,GPSSats,GPSDistance,Warnings,Horn"));
        _currentFile->flush();
    }

    return true;
}

void SdCardLogger::closeCurrentFile()
{
    if (_currentFile != nullptr && _fileOpen)
    {
        _currentFile->flush();
        MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
        sdDriver.closeFile(MicroSdFileHandle::Logger);
        _currentFile = nullptr;
        _fileOpen = false;
    }
}

void SdCardLogger::updateFileName(uint64_t now)
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

void SdCardLogger::checkForDateChange(uint64_t now)
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
        // Buffer is full, drop oldest record (overwrite tail)
        _recordsDropped++;
        _bufferTail = (_bufferTail + 1) % SD_BUFFER_SIZE;
        _bufferCount--;
    }
    
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
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    if (!sdDriver.isReady())
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

void SdCardLogger::checkSdCardStatus()
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
    MicroSdInitState state = sdDriver.getInitState();

    // Check if card became available
    if (state == MicroSdInitState::Initialized && (_sdCardErrorRaised || _sdCardMissingRaised))
    {
        // Clear any raised warnings
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
    // Check if card is missing or failed
    else if (state == MicroSdInitState::Failed || state == MicroSdInitState::NotInitialized)
    {
        // Close any open files
        if (_fileOpen)
        {
            closeCurrentFile();
        }

        // Assume card is missing (MicroSdDriver handles retries)
        if (!_sdCardMissingRaised)
        {
            _warningManager->raiseWarning(WarningType::SdCardMissing);
            _sdCardMissingRaised = true;
        }

        // Clear error warning if it was raised
        if (_sdCardErrorRaised)
        {
            _warningManager->clearWarning(WarningType::SdCardError);
            _sdCardErrorRaised = false;
        }
    }
}

bool SdCardLogger::isSdCardReady() const
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();
    return sdDriver.isReady();
}

uint32_t SdCardLogger::getCurrentLogFileSize() const
{
    if (_currentFile == nullptr || !_fileOpen)
    {
        return 0;
    }

    return _currentFile->fileSize();
}

void SdCardLogger::releaseSDCard()
{
    MicroSdDriver& sdDriver = MicroSdDriver::getInstance();

    if (!sdDriver.isReady())
    {
        return;
    }

    // Flush any pending writes first
    flush();

    // Close the current file (already done by flush, but ensure it)
    closeCurrentFile();
}
#endif