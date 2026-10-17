#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <stdint.h>

namespace BuildInfo
{
    constexpr char PanelType = 'C';
    constexpr uint8_t HardwareVariant = 1;
    constexpr uint8_t Major = 1;
    constexpr uint8_t Minor = 2;     // <--- V1.02: fix BOOTSEL pe core0 (portat din duplex 10.37)
    constexpr uint16_t Build = 102;

    // Amprente de trasabilitate capturate granular la momentul compilării fizice
    constexpr char Date[] = __DATE__;
    constexpr char Time[] = __TIME__;
    
    // Extrage automat versiunea curentă a compilatorului GCC ARM din Arduino IDE
    constexpr char Compiler[] = __VERSION__;

    // Slot persistent rezervat pentru Git Commit SHA-1 
    #ifdef GIT_REV
        constexpr char GitHash[] = GIT_REV;
    #else
        constexpr char GitHash[] = "N/A - Arduino IDE";
    #endif
}

#endif // BUILD_INFO_H
