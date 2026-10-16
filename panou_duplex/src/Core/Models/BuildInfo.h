#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <stdint.h>

namespace BuildInfo
{
    // Identitatea software unică a noii platforme de dezvoltare active
    constexpr uint8_t HardwareVariant = 1;
    constexpr uint8_t Major = 10;
    constexpr uint8_t Minor = 29;    // <--- Modificat oficial la versiunea V10.28
    constexpr uint16_t Build = 1029; // Contor binar intern actualizat pentru noul ciclu

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
