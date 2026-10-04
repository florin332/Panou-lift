// Hardware/Pins.h
// Maparea fizica pentru panoul de cabina cu un singur ILI9341.

#ifndef PINS_H
#define PINS_H

#include <stdint.h>

namespace Pins
{
    // =========================================================
    // ILI9341 - SPI0
    // =========================================================
    namespace TFT1 {
        constexpr uint8_t CS   = 22;
        constexpr uint8_t DC   = 21;
        constexpr uint8_t RST  = 20;
        constexpr uint8_t MOSI = 19;
        constexpr uint8_t SCK  = 18;
    }

    namespace UI {
        constexpr uint8_t BACKLIGHT = 26;
    }

    namespace UART {
        constexpr uint8_t ALARM_TX = 0;
        constexpr uint8_t UART1_RX = 1;
        constexpr uint8_t UART2_TX = 4;
        constexpr uint8_t DATA_RX = 5;
    }

    namespace Inputs {
        constexpr uint8_t ALARM = 7;
        constexpr uint8_t OVERLOAD = 8;
    }
}

#endif // PINS_H
