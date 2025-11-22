#include "SharedFunctions.h"


uint16_t SharedFunctions::freeMemory()
{
#if defined(ARDUINO_MEGA2560)
    extern int __heap_start, * __brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#elif defined(ARDUINO_UNO_R4)
    return 0;
#else
#error "You must define 'ARDUINO_MEGA2560' or 'ARDUINO_UNO_R4'"
#endif
}