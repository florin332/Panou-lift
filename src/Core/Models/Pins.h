//Hardware/Pins.h (Locația fizică a firelor în cupru)

#ifndef PINS_H
#define PINS_H

#include <stdint.h>

namespace Pins
{
    namespace TFT1 {
        constexpr uint8_t CS   = 0; 
        constexpr uint8_t DC   = 1; 
        constexpr uint8_t RST  = 2; 
        constexpr uint8_t MOSI = 3; 
        constexpr uint8_t SCK  = 6;
    }
    namespace TFT2 {
        constexpr uint8_t CS   = 11; 
        constexpr uint8_t DC   = 12; 
        constexpr uint8_t RST  = 13; 
        constexpr uint8_t MOSI = 15; 
        constexpr uint8_t SCK  = 14;
    }
    namespace UI {
        constexpr uint8_t BUTTON        = 7;  // GP7 Buton apel local hol
        constexpr uint8_t BUTTON_SVC    = 16; // GP16 Buton Meniu Service (Spate placă)
        constexpr uint8_t BUTTON_DEV    = 26; // GP26 Buton Meniu Developer (Spate placă)
        constexpr uint8_t LED_RED       = 5;  
        constexpr uint8_t LED_GREEN     = 8;  
        constexpr uint8_t BUZZER        = 10;
    }
    namespace RS485 {
        constexpr uint8_t TX_ENABLE = 22; 
        constexpr uint8_t LIFT1_RX  = 9;  
        constexpr uint8_t LIFT2_RX  = 17; 
        constexpr uint8_t CALL_RX   = 21; 
        constexpr uint8_t CALL_TX   = 28;
    }
}

#endif // PINS_H
