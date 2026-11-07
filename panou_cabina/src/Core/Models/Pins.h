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

    namespace Ethernet {
        constexpr uint8_t MISO = 12;
        constexpr uint8_t CS = 13;
        constexpr uint8_t SCK = 14;
        constexpr uint8_t MOSI = 15;
    }

    namespace Inputs {
        constexpr uint8_t ALARM = 7;
        constexpr uint8_t OVERLOAD = 6;
        constexpr uint8_t RESET = 11;
    }

    namespace Audio {
        constexpr uint8_t I2S_BCLK  = 8;
        constexpr uint8_t I2S_LRCLK = 9;
        constexpr uint8_t I2S_DATA  = 10;
    }
}

#endif // PINS_H
