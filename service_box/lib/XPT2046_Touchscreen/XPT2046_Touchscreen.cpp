#include "XPT2046_Touchscreen.h"

#define SPI_SETTING SPISettings(2000000, MSBFIRST, SPI_MODE0)

bool XPT2046_Touchscreen::begin() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    if (tirqPin < 255) {
        pinMode(tirqPin, INPUT_PULLUP);
    }
    return true;
}

bool XPT2046_Touchscreen::touched() {
    uint16_t x, y;
    uint8_t z;
    readData(&x, &y, &z);
    return z > 200; // Valoare implicită prag presiune (Z_THRESHOLD)
}

TS_Point XPT2046_Touchscreen::getPoint() {
    uint16_t x, y;
    uint8_t z;
    readData(&x, &y, &z);
    return TS_Point(x, y, z);
}

void XPT2046_Touchscreen::readData(uint16_t *x, uint16_t *y, uint8_t *z) {
    spi->beginTransaction(SPI_SETTING);
    digitalWrite(csPin, LOW);
    
    // Citire Axă X
    spi->transfer(0xD1);
    uint8_t x1 = spi->transfer(0x00);
    uint8_t x2 = spi->transfer(0x00);
    uint16_t rawX = ((x1 << 8) | x2) >> 3;

    // Citire Axă Y
    spi->transfer(0x91);
    uint8_t y1 = spi->transfer(0x00);
    uint8_t y2 = spi->transfer(0x00);
    uint16_t rawY = ((y1 << 8) | y2) >> 3;

    // Citire Presiune Z1
    spi->transfer(0xB1);
    uint8_t z1_1 = spi->transfer(0x00);
    uint8_t z1_2 = spi->transfer(0x00);
    uint16_t rawZ1 = ((z1_1 << 8) | z1_2) >> 3;

    // Citire Presiune Z2
    spi->transfer(0xC1);
    uint8_t z2_1 = spi->transfer(0x00);
    uint8_t z2_2 = spi->transfer(0x00);
    uint16_t rawZ2 = ((z2_1 << 8) | z2_2) >> 3;

    digitalWrite(csPin, HIGH);
    spi->endTransaction();

    // Calcul presiune grosieră
    if (rawZ1 > 0) {
        *z = (rawZ2 / rawZ1) * 10;
    } else {
        *z = 0;
    }

    // Aplicare rotație hardware direct la citire
    switch (rotation) {
        case 0:
            *x = rawX;
            *y = rawY;
            break;
        case 1:
            *x = 4095 - rawY;
            *y = rawX;
            break;
        case 2:
            *x = 4095 - rawX;
            *y = 4095 - rawY;
            break;
        case 3:
            *x = rawY;
            *y = 4095 - rawX;
            break;
    }
}

bool XPT2046_Touchscreen::bufferEmpty() {
    return true;
}

bool XPT2046_Touchscreen::tirqTouched() {
    if (tirqPin < 255) {
        return digitalRead(tirqPin) == LOW;
    }
    return false;
}

void XPT2046_Touchscreen::update() {
    // Funcție lăsată pentru compatibilitate cu apelurile ISR în fundal
}
