#ifdef SERVICEBOX_WAVESHARE
#include "HardwareInterface.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <CSE_CST328.h>

class HardwareWaveshare : public HardwareInterface {
private:
    // Pinii fixati hardware pe cablajul Waveshare RP2350 pentru ecranul ST7789
    Adafruit_ST7789 tft = Adafruit_ST7789(13, 14, 11, 10, 15); // CS, DC, MOSI, SCLK, RST
    CSE_CST328 touch;
    
    bool touchedState = false;
    int lastX = 0;
    int lastY = 0;

public:
    void init() override {
        // Inițializare ST7789 pentru placa Waveshare (Rezoluție 320x240)
        tft.init(240, 320); 
        tft.setRotation(1);
        tft.fillScreen(ST77XX_BLACK);
        
        // Inițializare touch capacitiv CST328 pe magistrala I2C (GP6=SDA, GP7=SCL)
        Wire.setSDA(6);
        Wire.setSCL(7);
        Wire.begin();
        touch.begin(Wire, 6, 7); // Transmitem obiectul Wire și pinii de I2C
        
        // Pornire UART0 implicit pe rpipico2 (Pini GP4/GP5)
        Serial1.setTX(4);
        Serial1.setRX(5);
        Serial1.begin(115200);
    }

    void updateTouch() override {
        // Verificare stare touch capacitiv CST328
        if (touch.available()) {
            touchedState = true;
            lastX = touch.getX();
            lastY = touch.getY();
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
