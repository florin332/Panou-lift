// src/Drivers/Custom_ST7735.h
#ifndef CUSTOM_ST7735_H
#define CUSTOM_ST7735_H

#include <Adafruit_ST7735.h>

class Custom_ST7735 : public Adafruit_ST7735 {
public:
    // Constructori care delegă către Adafruit_ST7735
    Custom_ST7735(int8_t cs, int8_t dc, int8_t rst)
        : Adafruit_ST7735(cs, dc, rst) {}

    Custom_ST7735(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst)
        : Adafruit_ST7735(spiClass, cs, dc, rst) {}

    // Setteri — apelează DUPĂ initR() !
    void setRowStart(int16_t row) { _rowstart = row; }
    void setColStart(int16_t col) { _colstart = col; }

    // Getters (opțional, pentru debug)
    int16_t getRowStart() const { return _rowstart; }
    int16_t getColStart() const { return _colstart; }
};

#endif // CUSTOM_ST7735_H