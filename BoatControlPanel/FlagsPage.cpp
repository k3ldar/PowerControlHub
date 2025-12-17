#include "FlagsPage.h"
#include "SystemFunctions.h"


// Nextion Names/Ids on current Page
constexpr uint8_t ButtonNext = 3;
constexpr uint8_t ButtonPrevious = 2;
constexpr uint8_t BtnPrevFlag = 7;
constexpr uint8_t BtnNextFlag = 6;
constexpr char FlagTextName[] = "flagText";
constexpr char FlagImageName[] = "flagImg";

constexpr uint8_t FlagFirst = 21;
constexpr uint8_t FlagLast = 60;
constexpr uint8_t FlagLetterA = 31;
constexpr uint8_t FlagLetterZ = 56;

// Store flag meanings in PROGMEM to save RAM
const char FlagMeaning_A[] PROGMEM = "Alfa: Diver down; keep\r\nwell clear at slow speed.";
const char FlagMeaning_B[] PROGMEM = "Bravo: I am taking in,\r\ndischarging, or carrying\r\ndangerous goods.";
const char FlagMeaning_C[] PROGMEM = "Charlie: Yes (affirmative).";
const char FlagMeaning_D[] PROGMEM = "Delta: Keep clear of me;\r\nI am maneuvering\r\nwith difficulty.";
const char FlagMeaning_E[] PROGMEM = "Echo: I am altering my\r\ncourse to port.";
const char FlagMeaning_F[] PROGMEM = "Foxtrot: I am disabled;\r\ncommunicate with me.";
const char FlagMeaning_G[] PROGMEM = "Golf: I require a pilot.\r\n(Fishing: I am hauling nets.)";
const char FlagMeaning_H[] PROGMEM = "Hotel: I have a pilot\r\non board.";
const char FlagMeaning_I[] PROGMEM = "India: I am altering my\r\ncourse to starboard.";
const char FlagMeaning_J[] PROGMEM = "Juliet: I am leaking\r\ndangerous goods.";
const char FlagMeaning_K[] PROGMEM = "Kilo: I wish to\r\ncommunicate with you.";
const char FlagMeaning_L[] PROGMEM = "Lima: You should stop\r\nyour vessel immediately.";
const char FlagMeaning_M[] PROGMEM = "Mike: My vessel is stopped;\r\nmaking no way.";
const char FlagMeaning_N[] PROGMEM = "November: No (negative).";
const char FlagMeaning_O[] PROGMEM = "Oscar: Man overboard.";
const char FlagMeaning_P[] PROGMEM = "Papa: All persons should\r\nreport on board; vessel is\r\nabout to proceed to sea.";
const char FlagMeaning_Q[] PROGMEM = "Quebec: My vessel is\r\nhealthy and I request\r\nfree pratique.";
const char FlagMeaning_R[] PROGMEM = "Romeo: No ICS meaning\r\n(used for signals/\r\nport facilities only).";
const char FlagMeaning_S[] PROGMEM = "Sierra: I am operating\r\nastern propulsion.";
const char FlagMeaning_T[] PROGMEM = "Tango: Keep clear of me;\r\nI am engaged in pair trawling.";
const char FlagMeaning_U[] PROGMEM = "Uniform: You are running\r\ninto danger.";
const char FlagMeaning_V[] PROGMEM = "Victor: I require assistance.";
const char FlagMeaning_W[] PROGMEM = "Whiskey: I require\r\nmedical assistance.";
const char FlagMeaning_X[] PROGMEM = "X-ray: Stop your intentions\r\nand watch for my signals.";
const char FlagMeaning_Y[] PROGMEM = "Yankee: My anchor is\r\ndragging.";
const char FlagMeaning_Z[] PROGMEM = "Zulu: I require a tug.\r\n(Fishing: I am shooting nets.)";

const char* const FlagMeanings[26] PROGMEM = {
    FlagMeaning_A, FlagMeaning_B, FlagMeaning_C, FlagMeaning_D,
    FlagMeaning_E, FlagMeaning_F, FlagMeaning_G, FlagMeaning_H,
    FlagMeaning_I, FlagMeaning_J, FlagMeaning_K, FlagMeaning_L,
    FlagMeaning_M, FlagMeaning_N, FlagMeaning_O, FlagMeaning_P,
    FlagMeaning_Q, FlagMeaning_R, FlagMeaning_S, FlagMeaning_T,
    FlagMeaning_U, FlagMeaning_V, FlagMeaning_W, FlagMeaning_X,
    FlagMeaning_Y, FlagMeaning_Z
};

constexpr unsigned long RefreshIntervalMs = 10000;


FlagsPage::FlagsPage(Stream* serialPort,
    WarningManager* warningMgr,
    SerialCommandManager* commandMgrLink,
    SerialCommandManager* commandMgrComputer)
    : BaseBoatPage(serialPort, warningMgr, commandMgrLink, commandMgrComputer),
    _currentFlagIndex(FlagLetterA)
{
}

void FlagsPage::begin()
{
    updateFlagDisplay();
}

void FlagsPage::refresh(unsigned long now)
{
    (void)now;
}

void FlagsPage::updateFlagDisplay()
{
    setPicture(FlagImageName, _currentFlagIndex);

    // Check if the flag is within the letter range (A-Z)
    if (_currentFlagIndex >= FlagLetterA && _currentFlagIndex <= FlagLetterZ)
    {
        uint8_t letterIndex = _currentFlagIndex - FlagLetterA;
        
        // Read the pointer from PROGMEM and convert to cha*
        const char* meaningPtr = (const char*)pgm_read_ptr(&FlagMeanings[letterIndex]);
        char buffer[128];
		strcpy_P(buffer, meaningPtr);

        sendText(FlagTextName, buffer);
    }
    else
    {
        // Not a letter flag, send empty string
        sendText(FlagTextName, "");
    }
}

void FlagsPage::cycleNextFlag()
{
    if (_currentFlagIndex >= FlagLast)
    {
        _currentFlagIndex = FlagFirst;
    }
    else
    {
        _currentFlagIndex++;
    }
    
    updateFlagDisplay();
}

void FlagsPage::cyclePrevFlag()
{
    if (_currentFlagIndex <= FlagFirst)
    {
        _currentFlagIndex = FlagLast;
    }
    else
    {
        _currentFlagIndex--;
    }
    
    updateFlagDisplay();
}

// Handle touch events for buttons
void FlagsPage::handleTouch(uint8_t compId, uint8_t eventType)
{
    (void)eventType;

    switch (compId)
    {
    case BtnPrevFlag:
        cyclePrevFlag();
        break;

    case BtnNextFlag:
        cycleNextFlag();
        break;

    case ButtonNext:
        setPage(PageCardinalMarkers);
        return;

    case ButtonPrevious:
        setPage(PageSoundSignals);
        return;
    }
}
