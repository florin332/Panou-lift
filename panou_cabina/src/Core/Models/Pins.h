// Hardware/Pins.h
// Maparea fizică actuală a pinilor pentru panoul dual-TFT

#ifndef PINS_H
#define PINS_H

#include <stdint.h>

namespace Pins
{
    // =========================================================
    // TFT 1 - SPI0
    // =========================================================
    namespace TFT1 {
        constexpr uint8_t CS   = 0;
        constexpr uint8_t DC   = 1;
        constexpr uint8_t RST  = 2;
        constexpr uint8_t MOSI = 3;
        constexpr uint8_t SCK  = 6;
    }

    // =========================================================
    // TFT 2 - SPI1
    // =========================================================
    namespace TFT2 {
        constexpr uint8_t CS   = 11;
        constexpr uint8_t DC   = 12;
        constexpr uint8_t RST  = 13;
        constexpr uint8_t MOSI = 15;
        constexpr uint8_t SCK  = 14;
    }

    // =========================================================
    // UI / PANOU
    // =========================================================
    namespace UI {
        constexpr uint8_t BUTTON      = 7;   // Buton apel local hol
        constexpr uint8_t BACKLIGHT   = 4;   // Backlight comun TFT1 + TFT2

        constexpr uint8_t RCWL        = 16;  // Ieșire RCWL-0516
        constexpr uint8_t BUTTON_SVC    = 18;  // Buton Meniu Service
        constexpr uint8_t BUTTON_DEV  = 26;  // Buton Developer

        constexpr uint8_t LED_RED     = 5;
        constexpr uint8_t LED_GREEN   = 8;
        constexpr uint8_t BUZZER      = 10;
    }

    // =========================================================
    // RS485 / COMUNICAȚII
    // =========================================================
    namespace RS485 {
        constexpr uint8_t TX_ENABLE = 22;

        constexpr uint8_t LIFT1_RX  = 9;
        constexpr uint8_t LIFT2_RX  = 17;

        constexpr uint8_t CALL_RX   = 21;
        constexpr uint8_t CALL_TX   = 28;
    }
}

#endif // PINS_H
