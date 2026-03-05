// 
// 
// 

#include "Local.h"

#if defined(SD_CARD_SUPPORT)

#include "MicroSdDriver.h"
#include "WarningManager.h"
#include <SPI.h>

// Initialize static singleton instance
MicroSdDriver* MicroSdDriver::_instance = nullptr;

MicroSdDriver::MicroSdDriver()
    : _warningManager(nullptr),
      _onCardReadyCallback(nullptr),
      _csPin(0),
      _speedMhz(4),
      _initState(MicroSdInitState::NotInitialized),
      _cardPresent(false),
      _exclusiveAccessActive(false),
      _openFileCount(0),
      _totalSize(0),
      _freeSpace(0),
      _lastPresenceCheck(0),
      _cardSerialNumber(0),
      _lastFreeSpaceUpdate(0),
      _lastInitAttemptTime(0)
{
}

MicroSdDriver& MicroSdDriver::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new MicroSdDriver();
    }
    return *_instance;
}

void MicroSdDriver::setWarningManager(WarningManager* warningManager)
{
    _warningManager = warningManager;
}

void MicroSdDriver::setOnCardReadyCallback(SdCardEventCallback callback)
{
    _onCardReadyCallback = callback;
}

void MicroSdDriver::beginInitialize(uint8_t csPin, uint32_t speedMhz)
{
    _csPin = csPin;
    _speedMhz = speedMhz;
    _initState = MicroSdInitState::Initializing;
    _lastInitAttemptTime = 0;
    _cardPresent = false;

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    SPI.begin();
}

void MicroSdDriver::reinitialize()
{
    closeAllFiles();
    _initState = MicroSdInitState::NotInitialized;
    _cardPresent = false;
    beginInitialize(_csPin, _speedMhz);
}

bool MicroSdDriver::isCardPresent(bool forceCheck)
{
    if (!forceCheck)
    {
        return _cardPresent;
    }

    // Force hardware check (can be expensive)
    if (_initState != MicroSdInitState::Initialized)
    {
        return false;
    }

    // Try to access card - if this fails, card is likely missing or has errors
    uint8_t errorCode = _sd.card()->errorCode();

    if (errorCode != 0)
    {
        // Card has communication errors
        _cardPresent = false;

        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::SdCardError);
        }

        return false;
    }

    // Card is responding correctly
    if (_warningManager)
    {
        _warningManager->clearWarning(WarningType::SdCardError);
    }

    _cardPresent = true;
    return true;
}

FsFile* MicroSdDriver::openFile(MicroSdFileHandle handle, const char* fileName, oflag_t oflag)
{
    if (!isValidHandle(handle) || fileName == nullptr || _initState != MicroSdInitState::Initialized)
    {
        return nullptr;
    }

    if (_exclusiveAccessActive && handle != MicroSdFileHandle::ConfigLoader)
    {
        return nullptr;
    }

    // Check if file is already open with this handle
    SdFileInfo* fileInfo = findFileInfo(handle);
    if (fileInfo != nullptr && fileInfo->isOpen)
    {
        // Close existing file first
        closeFile(handle);
    }

    // Find available slot
    fileInfo = findFileInfo(handle);
    if (fileInfo == nullptr)
    {
        // Find empty slot
        for (uint8_t i = 0; i < SdMaximumOpenFiles; i++)
        {
            if (!_files[i].isOpen)
            {
                fileInfo = &_files[i];
                fileInfo->handle = handle;
                break;
            }
        }
    }

    if (fileInfo == nullptr)
    {
        // No available slots
        return nullptr;
    }

    // Attempt to open file
    if (!fileInfo->file.open(fileName, oflag))
    {
        // File open failed - raise SD card error warning
        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::SdCardError);
        }
        return nullptr;
    }

    // Successfully opened - clear any previous error
    if (_warningManager)
    {
        _warningManager->clearWarning(WarningType::SdCardError);
    }

    fileInfo->isOpen = true;
    strncpy(fileInfo->fileName, fileName, sizeof(fileInfo->fileName) - 1);
    fileInfo->fileName[sizeof(fileInfo->fileName) - 1] = '\0';
    _openFileCount++;

    return &fileInfo->file;
}

bool MicroSdDriver::closeFile(MicroSdFileHandle handle)
{
    if (!isValidHandle(handle))
    {
        return false;
    }

    SdFileInfo* fileInfo = findFileInfo(handle);
    if (fileInfo == nullptr || !fileInfo->isOpen)
    {
        return false;
    }

    fileInfo->file.close();
    fileInfo->isOpen = false;
    fileInfo->handle = MicroSdFileHandle::Invalid;
    memset(fileInfo->fileName, 0, sizeof(fileInfo->fileName));
    _openFileCount--;

    return true;
}

void MicroSdDriver::closeAllFiles()
{
    for (uint8_t i = 0; i < SdMaximumOpenFiles; i++)
    {
        if (_files[i].isOpen)
        {
            _files[i].file.close();
            _files[i].isOpen = false;
            _files[i].handle = MicroSdFileHandle::Invalid;
            memset(_files[i].fileName, 0, sizeof(_files[i].fileName));
        }
    }
    _openFileCount = 0;
}

bool MicroSdDriver::isFileOpen(MicroSdFileHandle handle) const
{
    if (!isValidHandle(handle))
    {
        return false;
    }

    for (uint8_t i = 0; i < SdMaximumOpenFiles; i++)
    {
        if (_files[i].handle == handle && _files[i].isOpen)
        {
            return true;
        }
    }

    return false;
}

bool MicroSdDriver::fileExists(const char* fileName)
{
    if (_initState != MicroSdInitState::Initialized || fileName == nullptr)
    {
        return false;
    }

    return _sd.exists(fileName);
}

bool MicroSdDriver::deleteFile(const char* fileName)
{
    if (_initState != MicroSdInitState::Initialized || fileName == nullptr)
    {
        return false;
    }

    return _sd.remove(fileName);
}

uint64_t MicroSdDriver::getFreeSpace(bool forceUpdate)
{
    if (_initState != MicroSdInitState::Initialized)
    {
        return 0;
    }

    if (forceUpdate)
    {
        updateCardInfo();
    }

    return _freeSpace;
}

bool MicroSdDriver::requestExclusiveAccess()
{
    if (_initState != MicroSdInitState::Initialized)
    {
        return false;
    }

    // Close all files to ensure exclusive access
    closeAllFiles();
    _exclusiveAccessActive = true;
    return true;
}

void MicroSdDriver::releaseExclusiveAccess()
{
    _exclusiveAccessActive = false;
}

void MicroSdDriver::update(unsigned long now)
{
    // Handle non-blocking initialization
    if (_initState == MicroSdInitState::Initializing)
    {
        // Attempt SD card initialization at configured speed
        if (_sd.begin(_csPin, SD_SCK_MHZ(_speedMhz)))
        {
            _initState = MicroSdInitState::Settling;
            _cardPresent = true;
            _lastInitAttemptTime = now;

            // Read card serial number for change detection
            _cardSerialNumber = readCardSerialNumber();

            // Clear SD card warnings - card initialized successfully
            if (_warningManager)
            {
                _warningManager->clearWarning(WarningType::SdCardMissing);
                _warningManager->clearWarning(WarningType::SdCardError);
            }
        }
        else
        {
            // Initialization failed - mark as failed and will retry later
            _initState = MicroSdInitState::Failed;
            _cardPresent = false;
            _lastInitAttemptTime = now;

            // Raise SD card missing warning
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::SdCardMissing);
            }
        }

        return;
    }

    // Handle settling period - let card stabilize before full use
    if (_initState == MicroSdInitState::Settling)
    {
        if (now - _lastInitAttemptTime >= SdCardSettlingDelayMs)
        {
            _initState = MicroSdInitState::Initialized;

            // Card has settled - now safe to do expensive operations
            updateCardInfo(false);

            // Check free space and raise warning if below 10%
            checkFreeSpaceWarning();

            // Notify callback that card is ready (first initialization, not a swap)
            if (_onCardReadyCallback)
            {
                _onCardReadyCallback(false);
            }
        }
        return;
    }

    // Auto-retry if initialization failed (using same interval as presence check)
    if (_initState == MicroSdInitState::Failed)
    {
        if (now - _lastInitAttemptTime >= SdCardPresenceCheckMs)
        {
            reinitialize();
        }
        return;
    }

    // Normal operation - periodic card presence check
    if (_initState == MicroSdInitState::Initialized)
    {
        if (now - _lastPresenceCheck >= SdCardPresenceCheckMs)
        {
            // Check if card is still present
            if (!isCardPresent(true))
            {
                // Card removed - reset state
                closeAllFiles();
                _initState = MicroSdInitState::Failed;
                _cardSerialNumber = 0;
                _lastFreeSpaceUpdate = 0;

                // Raise SD card missing warning, clear others (no card = no errors/low space)
                if (_warningManager)
                {
                    _warningManager->raiseWarning(WarningType::SdCardMissing);
                    _warningManager->clearWarning(WarningType::SdCardError);
                    _warningManager->clearWarning(WarningType::SdCardLowSpace);
                }
            }
            else
            {
                // Card present - check if it's the same card
                uint32_t currentSerial = readCardSerialNumber();

                if (_cardSerialNumber != 0 && currentSerial != 0 && currentSerial != _cardSerialNumber)
                {
                    // Different card inserted! Close all files and re-initialize
                    closeAllFiles();
                    _cardSerialNumber = currentSerial;
                    _lastFreeSpaceUpdate = 0;
                    updateCardInfo(true); // Force full recalculation for new card

                    // Check free space on new card
                    checkFreeSpaceWarning();

                    // Notify callback that a new card is ready
                    if (_onCardReadyCallback)
                    {
                        _onCardReadyCallback(true);
                    }
                }
            }

            _lastPresenceCheck = now;
        }
    }
}

SdFileInfo* MicroSdDriver::findFileInfo(MicroSdFileHandle handle)
{
    for (uint8_t i = 0; i < SdMaximumOpenFiles; i++)
    {
        if (_files[i].handle == handle)
        {
            return &_files[i];
        }
    }
    return nullptr;
}

bool MicroSdDriver::isValidHandle(MicroSdFileHandle handle) const
{
    return handle != MicroSdFileHandle::Invalid && 
           static_cast<uint8_t>(handle) < SdMaximumOpenFiles;
}

void MicroSdDriver::updateCardInfo(bool forceExpensiveCheck)
{
    if (_initState != MicroSdInitState::Initialized)
    {
        return;
    }

    // Fast: Read total size from FAT32 boot sector (single 512-byte sector read)
    _totalSize = readTotalSizeFromBootSector();

    if (_totalSize == 0)
    {
        // Failed to read card size - card error
        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::SdCardError);
        }
        return;
    }

    // Expensive: Only recalculate free space if forced or cache is stale (> 1 hour)
    unsigned long now = millis();
    bool freeSpaceStale = (now - _lastFreeSpaceUpdate) >= SdFreeSpaceCacheMs;

    if (forceExpensiveCheck || freeSpaceStale || _lastFreeSpaceUpdate == 0)
    {
        // This is the expensive operation (200-1000ms on large cards)
        uint32_t freeClusterCount = _sd.freeClusterCount();
        uint32_t sectorsPerCluster = _sd.sectorsPerCluster();

        if (freeClusterCount == 0 && sectorsPerCluster == 0)
        {
            // Failed to read free space - card error
            if (_warningManager)
            {
                _warningManager->raiseWarning(WarningType::SdCardError);
            }
            return;
        }

        _freeSpace = static_cast<uint64_t>(freeClusterCount) * sectorsPerCluster * 512ULL;
        _lastFreeSpaceUpdate = now;

        // Successfully read card info - clear error warning
        if (_warningManager)
        {
            _warningManager->clearWarning(WarningType::SdCardError);
        }

        // Check if free space warning should be raised/cleared
        checkFreeSpaceWarning();
    }
}

uint32_t MicroSdDriver::readCardSerialNumber()
{
    if (_initState != MicroSdInitState::Initialized && _initState != MicroSdInitState::Settling)
    {
        return 0;
    }

    cid_t cid;

    if (!_sd.card()->readCID(&cid))
    {
        // Failed to read CID - card communication error
        if (_warningManager)
        {
            _warningManager->raiseWarning(WarningType::SdCardError);
        }
        return 0;
    }

    // Successfully read CID - clear error warning
    if (_warningManager)
    {
        _warningManager->clearWarning(WarningType::SdCardError);
    }

    return cid.psn(); // Product Serial Number (32-bit unique ID)
}

uint64_t MicroSdDriver::readTotalSizeFromBootSector()
{
    if (_initState != MicroSdInitState::Initialized)
    {
        return 0;
    }

    // Fast method: Use SdFat's built-in functions that already read boot sector
    uint32_t cardSizeBlocks = _sd.card()->sectorCount();
    return static_cast<uint64_t>(cardSizeBlocks) * 512ULL;
}

void MicroSdDriver::checkFreeSpaceWarning()
{
    if (!_warningManager || _totalSize == 0)
    {
        return;
    }

    // Calculate percentage of free space
    // Use integer math to avoid floating point: (freeSpace * 100) / totalSize
    uint64_t freePercentage = (_freeSpace * 100ULL) / _totalSize;

    if (freePercentage < 10)
    {
        // Free space below 10% - raise warning
        _warningManager->raiseWarning(WarningType::SdCardLowSpace);
    }
    else
    {
        // Free space OK - clear warning
        _warningManager->clearWarning(WarningType::SdCardLowSpace);
    }
}

#endif