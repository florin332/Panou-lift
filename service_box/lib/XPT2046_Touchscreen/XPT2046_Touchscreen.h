#ifndef _XPT2046_Touchscreen_h_
#define _XPT2046_Touchscreen_h_

#include <Arduino.h>
#include <SPI.h>

class TS_Point {
public:
    constexpr TS_Point() : x(0), y(0), z(0) {}
    constexpr TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
    bool operator==(const TS_Point& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }
    bool operator!=(const TS_Point& rhs) const { return !(*this == rhs); }
    int16_t x, y, z;
};

class XPT2046_Touchscreen {
public:
    // Constructorul modificat propus de tine care acceptă pointer către interfața SPI
    constexpr XPT2046_Touchscreen(uint8_t cspin, uint8_t tirq = 255, SPIClass *spi = &SPI)
        : csPin(cspin), tirqPin(tirq), rotation(1), spi(spi) { }

    bool begin();
    TS_Point getPoint();
    bool tirqTouched();
    bool touched();
    void readData(uint16_t *x, uint16_t *y, uint8_t *z);
    bool bufferEmpty();
    uint8_t bufferSize() { return 1; }
    void setRotation(uint8_t n) { rotation = n % 4; }

    volatile bool isrWake = true;

private:
    void update();
    uint8_t csPin, tirqPin, rotation;
    SPIClass *spi; // Pointerul către magistrala alocată dinamic
    int16_t xraw = 0, yraw = 0, zraw = 0;
    uint32_t msraw = 0x80000000;
};

#endif
