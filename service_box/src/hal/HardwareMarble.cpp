#include "HardwareInterface.h"
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <SD.h>

#ifdef SERVICEBOX_MARBLE

// Instanțiere curată: CS, IRQ (255 = dezactivat), și pointer către magistrala SPI1
XPT2046_Touchscreen ts(TOUCH_CS, 255, &SPI1);

class HardwareMarble : public HardwareInterface {
private:
    int _touchX = 0;
    int _touchY = 0;
    bool _isTouched = false;

public:
    void init() override {
        // 1. Inițializare UART1 cu pini custom
        DISPLAY_UART.setTX(SERIAL_TX_PIN);
        DISPLAY_UART.setRX(SERIAL_RX_PIN);
        DISPLAY_UART.begin(115200);

        // 2. Gestionare flexibilă pentru Backlight
        #if defined(TFT_BL) && (TFT_BL >= 0)
            pinMode(TFT_BL, OUTPUT);
            digitalWrite(TFT_BL, HIGH); // Aprinde ecranul
        #endif
        
        // 3. Configurare SPI0 dedicat exclusiv pentru Cardul SD
        SPI.setTX(SD_MOSI);
        SPI.setRX(SD_MISO);
        SPI.setSCK(SD_SCLK);
        SPI.begin(); 

        // 4. Configurare SPI1 dedicat pentru Display și Touch
        SPI1.setTX(TFT_MOSI);
        SPI1.setRX(TOUCH_MISO);
        SPI1.setSCK(TFT_SCK);
        SPI1.begin(); 

        // 5. Pornire Card SD pe magistrala standard SPI (SPI0)
        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);
        SD.begin(SD_CS, SPI); // Rulează nativ pe SPI0 configurat mai sus

        // 6. Pornire Touch pe magistrala SPI1 (configurată din constructor)
        ts.begin(); 
        ts.setRotation(1); 
    }

    void updateTouch() override {
        // Verificăm dacă ecranul este atins în mod activ
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            
            // Salvăm coordonatele citite hardware
            _touchX = p.x;
            _touchY = p.y;
            _isTouched = true;
        } else {
            _isTouched = false;
        }
    }

    bool isScreenTouched() override {
        return _isTouched;
    }

    int getTouchX() override {
        return _touchX;
    }

    int getTouchY() override {
        return _touchY;
    }

    void sendProtocolData(uint8_t* data, uint16_t len) override {
        // Trimite datele prin UART-ul mapat pentru Marble Pico
        DISPLAY_UART.write(data, len);
    }
};

// Instanțierea clasei specifice Marble Pico
HardwareMarble marbleInstance;

// Legarea referinței globale unice "Hardware" de instanța Marble
HardwareInterface& Hardware = marbleInstance;

#endif // SERVICEBOX_MARBLE
