#ifdef SERVICEBOX_WAVESHARE

#include "HardwareInterface.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <CSE_CST328.h>

// ============================================================
// WAVESHARE RP2350 TOUCH LCD 2.8"
// ============================================================
//
// Display: ST7789T3 240x320
// Touch:   CST328 capacitive
//
// ---------------- DISPLAY SPI ----------------
// CS   = GPIO13
// DC   = GPIO16
// MOSI = GPIO11
// SCLK = GPIO10
// RST  = GPIO14
// BL   = GPIO15
//
// ---------------- TOUCH I2C -------------------
// SDA  = GPIO6
// SCL  = GPIO7
// INT  = GPIO17
// RST  = GPIO18
//
// ---------------- UART0 -----------------------
// TX   = GPIO4
// RX   = GPIO5
// ============================================================

class HardwareWaveshare : public HardwareInterface {
private:

    // ST7789
    Adafruit_ST7789 tft =
        Adafruit_ST7789(
            13,   // CS
            16,   // DC
            11,   // MOSI
            10,   // SCLK
            14    // RST
        );

    // CST328
    CSE_CST328 touch =
        CSE_CST328(
            240,      // touch width
            320,      // touch height
            &Wire,    // I2C bus
            18,       // RST
            17        // INT
        );

    bool touchedState = false;
    int lastX = 0;
    int lastY = 0;

public:

    void init() override {

        // ----------------------------------------------------
        // DISPLAY
        // ----------------------------------------------------

        tft.init(240, 320);

        tft.setRotation(1);

        tft.fillScreen(ST77XX_BLACK);


        // ----------------------------------------------------
        // TOUCH I2C
        // ----------------------------------------------------

        Wire.setSDA(6);
        Wire.setSCL(7);
        Wire.begin();

        touch.begin();

        // Touch coordinates must follow display rotation.
        touch.setRotation(1);


        // ----------------------------------------------------
        // UART0 / RS485
        // ----------------------------------------------------

        Serial1.setTX(4);
        Serial1.setRX(5);
        Serial1.begin(115200);
    }


    void updateTouch() override {

        // Read complete CST328 touch frame.
        touch.readData();

        if (touch.isTouched()) {

            CSE_TouchPoint point = touch.getPoint(0);

            touchedState = true;

            lastX = point.x;
            lastY = point.y;

        } else {

            touchedState = false;
        }
    }


    bool isScreenTouched() override {
        return touchedState;
    }


    int getTouchX() override {
        return lastX;
    }


    int getTouchY() override {
        return lastY;
    }


    void sendProtocolData(uint8_t* data, uint16_t len) override {
        Serial1.write(data, len);
    }
};


// ============================================================
// GLOBAL HARDWARE INSTANCE
// ============================================================

HardwareWaveshare waveshareInstance;

HardwareInterface& Hardware = waveshareInstance;

#endif



