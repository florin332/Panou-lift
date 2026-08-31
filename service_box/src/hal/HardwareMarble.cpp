#ifdef SERVICEBOX_MARBLE
#include "HardwareInterface.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

namespace TFT1 {
    constexpr uint8_t CS   = 0;
    constexpr uint8_t DC   = 1;
    constexpr uint8_t RST  = 2;
    constexpr uint8_t MOSI = 3;
    constexpr uint8_t SCK  = 6;
    
    // Alocare pini auxiliari pentru Touch SPI rezistiv (XPT2046)
    constexpr uint8_t TOUCH_CS = 7;
    constexpr uint8_t TOUCH_MISO = 4; 
}

class HardwareMarble : public HardwareInterface {
private:
    // Inițializare display folosind pinii stabili definiți de tine
    Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT1::CS, TFT1::DC, TFT1::MOSI, TFT1::SCK, TFT1::RST, -1);
    // Inițializare touch rezistiv XPT2046
    XPT2046_Touchscreen touch = XPT2046_Touchscreen(TFT1::TOUCH_CS);
    
    bool touchedState = false;
    int lastX = 0;
    int lastY = 0;

public:
    void init() override {
        // Pornire ecran ILI9341/ILI9340
        tft.begin();
        tft.setRotation(1); // Mod Landscape
        tft.fillScreen(ILI9341_BLACK);
        
        // Pornire Touch
        touch.begin();
        touch.setRotation(1);
        
        // Pornire UART1 pentru comunicație cu panoul duplex (Pini GP20/GP21)
        Serial1.setTX(20);
        Serial1.setRX(21);
        Serial1.begin(115200);
    }

    void updateTouch() override {
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            touchedState = true;
            // Mapare coordonate brute (specifice ecranului din Drátek) în pixeli (320x240)
            lastX = map(p.x, 200, 3800, 0, 320);
            lastY = map(p.y, 300, 3700, 0, 240);
        } else {
            touchedState = false;
        }
    }

    bool isScreenTouched() override { return touchedState; }
    int getTouchX() override { return lastX; }
    int getTouchY() override { return lastY; }
    
    void sendProtocolData(uint8_t* data, uint16_t len) override {
        Serial1.write(data, len);
    }
};

HardwareMarble marbleInstance;
HardwareInterface& Hardware = marbleInstance;
#endif

