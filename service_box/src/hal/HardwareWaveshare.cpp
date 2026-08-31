#ifdef SERVICEBOX_WAVESHARE
#include "HardwareInterface.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Wire.h>
#include <CSE_CST328.h> 

class HardwareWaveshare : public HardwareInterface {
private:
    Adafruit_ST7789 tft = Adafruit_ST7789(13, 14, 11, 10, 15);
    
    // Configurare constructor: latime, inaltime, magistrala Wire, pin reset, pin intrerupere
    CSE_CST328 touch = CSE_CST328(320, 240, &Wire, -1, 17); 
    
    bool touchedState = false;
    int lastX = 0;
    int lastY = 0;

public:
    void init() override {
        tft.init(240, 320);
        tft.setRotation(1);
        tft.fillScreen(ST77XX_BLACK);
        
        // Alocare pini I2C1 pentru Waveshare RP2350 (SDA=GP6, SCL=GP7)
        Wire.setSDA(6);
        Wire.setSCL(7);
        Wire.begin();
        
        // Initializare fara argumente
        touch.begin(); 
        
        Serial1.setTX(4);
        Serial1.setRX(5);
        Serial1.begin(115200);
    }

    void updateTouch() override {
        // In biblioteca CSE_CST328, starea este interogata prin functia isTouched()
        if (touch.isTouched()) {
            touchedState = true;
            // Coordonatele se extrag direct din matricea publica touchPoints a obiectului
            lastX = touch.touchPoints[0].x;
            lastY = touch.touchPoints[0].y;
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

HardwareWaveshare waveshareInstance;
HardwareInterface& Hardware = waveshareInstance;
#endif



