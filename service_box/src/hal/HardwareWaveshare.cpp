#include "HardwareInterface.h"

#include <Arduino.h>
#include <SPI.h>

#define BAT_EN 26

#ifdef SERVICEBOX_WAVESHARE

// ============================================================
// WAVESHARE RP2350 TOUCH LCD 2.8"
// MINIMAL LCD TEST
//
// GPIO + SPI1 + ST7789 + backlight
//
// FARA:
//   - Touch
//   - I2C
//   - SD
//   - UART service
//   - Service Box rendering
// ============================================================

class HardwareWaveshare : public HardwareInterface
{
private:

    // --------------------------------------------------------
    // LCD low-level
    // --------------------------------------------------------

    void lcdWriteCmd(uint8_t cmd)
    {
        digitalWrite(TFT_DC, LOW);
        digitalWrite(TFT_CS, LOW);

        SPI1.transfer(cmd);

        digitalWrite(TFT_CS, HIGH);
    }

    void lcdWriteData(uint8_t data)
    {
        digitalWrite(TFT_DC, HIGH);
        digitalWrite(TFT_CS, LOW);

        SPI1.transfer(data);

        digitalWrite(TFT_CS, HIGH);
    }

    // --------------------------------------------------------
    // ST7789 initialization
    //
    // Same register sequence as demo-rp2030
    // LCD_2IN8_Init(HORIZONTAL)
    // --------------------------------------------------------

    void initLCD()
    {
        Serial.println("[LCD] Reset...");

        // Hardware reset
        digitalWrite(TFT_RST, HIGH);
        delay(100);

        digitalWrite(TFT_RST, LOW);
        delay(100);

        digitalWrite(TFT_RST, HIGH);
        delay(100);

        // Sleep Out
        lcdWriteCmd(0x11);
        delay(120);

        // Memory Access Control
        // HORIZONTAL
        lcdWriteCmd(0x36);
        lcdWriteData(0x00);

        // Pixel Format
        lcdWriteCmd(0x3A);
        lcdWriteData(0x05);

        // Porch Setting
        lcdWriteCmd(0xB2);
        lcdWriteData(0x0C);
        lcdWriteData(0x0C);
        lcdWriteData(0x00);
        lcdWriteData(0x33);
        lcdWriteData(0x33);

        // Gate Control
        lcdWriteCmd(0xB7);
        lcdWriteData(0x75);

        // VCOM Setting
        lcdWriteCmd(0xBB);
        lcdWriteData(0x1A);

        // LCM Control
        lcdWriteCmd(0xC0);
        lcdWriteData(0x2C);

        // VDV and VRH Command Enable
        lcdWriteCmd(0xC2);
        lcdWriteData(0x01);
        lcdWriteData(0xFF);

        // VRHS
        lcdWriteCmd(0xC3);
        lcdWriteData(0x13);

        // VDV Set
        lcdWriteCmd(0xC4);
        lcdWriteData(0x20);

        // Frame Rate Control
        lcdWriteCmd(0xC6);
        lcdWriteData(0x0F);

        // Power Control 1
        lcdWriteCmd(0xD0);
        lcdWriteData(0xA4);
        lcdWriteData(0xA1);

        // Positive Voltage Gamma
        lcdWriteCmd(0xD6);
        lcdWriteData(0xA1);

        // Positive Gamma
        lcdWriteCmd(0xE0);

        lcdWriteData(0xD0);
        lcdWriteData(0x0D);
        lcdWriteData(0x14);
        lcdWriteData(0x0D);
        lcdWriteData(0x0D);
        lcdWriteData(0x09);
        lcdWriteData(0x38);
        lcdWriteData(0x44);
        lcdWriteData(0x4E);
        lcdWriteData(0x3A);
        lcdWriteData(0x17);
        lcdWriteData(0x18);
        lcdWriteData(0x2F);
        lcdWriteData(0x30);

        // Negative Gamma
        lcdWriteCmd(0xE1);

        lcdWriteData(0xD0);
        lcdWriteData(0x09);
        lcdWriteData(0x0F);
        lcdWriteData(0x08);
        lcdWriteData(0x07);
        lcdWriteData(0x14);
        lcdWriteData(0x37);
        lcdWriteData(0x44);
        lcdWriteData(0x4D);
        lcdWriteData(0x38);
        lcdWriteData(0x15);
        lcdWriteData(0x16);
        lcdWriteData(0x2C);
        lcdWriteData(0x2E);

        // Display Inversion ON
        lcdWriteCmd(0x21);

        // Display ON
        lcdWriteCmd(0x29);

        // Memory Write
        lcdWriteCmd(0x2C);

        Serial.println("[LCD] Initialization complete");
    }

    // --------------------------------------------------------
    // Set window
    //
    // Logic equivalent to:
    // LCD_2IN8_SetWindows()
    //
    // Coordinates:
    //   Xstart ... Xend-1
    //   Ystart ... Yend-1
    // --------------------------------------------------------

    void lcdSetWindow(
        uint16_t Xstart,
        uint16_t Ystart,
        uint16_t Xend,
        uint16_t Yend
    )
    {
        // Column Address Set
        lcdWriteCmd(0x2A);

        lcdWriteData(Xstart >> 8);
        lcdWriteData(Xstart & 0xFF);

        lcdWriteData((Xend - 1) >> 8);
        lcdWriteData((Xend - 1) & 0xFF);

        // Row Address Set
        lcdWriteCmd(0x2B);

        lcdWriteData(Ystart >> 8);
        lcdWriteData(Ystart & 0xFF);

        lcdWriteData((Yend - 1) >> 8);
        lcdWriteData((Yend - 1) & 0xFF);

        // Memory Write
        lcdWriteCmd(0x2C);
    }

    // --------------------------------------------------------
    // Fill entire horizontal display
    //
    // Waveshare LCD:
    //   width  = 320
    //   height = 240
    //
    // Same window convention as demo.
    // --------------------------------------------------------

    void fillScreen(uint16_t color)
    {
        const uint16_t width  = 320;
        const uint16_t height = 240;

        Serial.println("[LCD] Set full-screen window...");

        lcdSetWindow(
            0,
            0,
            width,
            height
        );

        Serial.println("[LCD] Filling screen...");

        digitalWrite(TFT_DC, HIGH);
        digitalWrite(TFT_CS, LOW);

        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;

        const uint32_t pixels =
            (uint32_t)width * height;

        for (uint32_t i = 0; i < pixels; ++i)
        {
            SPI1.transfer(hi);
            SPI1.transfer(lo);
        }

        digitalWrite(TFT_CS, HIGH);

        Serial.println("[LCD] Fill complete");
    }

public:

    // ========================================================
    // HardwareInterface
    // ========================================================

    void init() override
    {
        Serial.begin(115200);
        delay(100);

        Serial.println();
        Serial.println("========================================");
        Serial.println(" WAVESHARE LCD MINIMAL TEST");
        Serial.println("========================================");

        // ----------------------------------------------------
        // GPIO
        // Same relevant initialization as demo
        // ----------------------------------------------------

        pinMode(TFT_RST, OUTPUT);
        pinMode(TFT_DC,  OUTPUT);
        pinMode(TFT_CS,  OUTPUT);
        pinMode(BAT_EN,  OUTPUT);

        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TFT_DC, LOW);
        digitalWrite(BAT_EN, HIGH);

        Serial.println("[GPIO] OK");

        // ----------------------------------------------------
        // SPI1
        // Exact Waveshare demo mapping
        // ----------------------------------------------------

        SPI1.setRX(TFT_MISO);
        SPI1.setCS(TFT_CS);
        SPI1.setSCK(TFT_SCLK);
        SPI1.setTX(TFT_MOSI);

        SPI1.begin();

        SPI1.beginTransaction(
            SPISettings(
                66500000,
                MSBFIRST,
                SPI_MODE0
            )
        );

        Serial.println("[SPI1] OK @ 66.5 MHz");

        // ----------------------------------------------------
        // LCD initialization
        // ----------------------------------------------------

        initLCD();

        // ----------------------------------------------------
        // Backlight
        // Equivalent to DEV_SET_PWM(100)
        // ----------------------------------------------------

        pinMode(TFT_BL, OUTPUT);
        analogWrite(TFT_BL, 255);

        Serial.println("[BL] ON");

        // ----------------------------------------------------
        // Test actual GRAM addressing
        // ----------------------------------------------------

        fillScreen(0xF800);

        Serial.println();
        Serial.println("========================================");
        Serial.println(" TEST COMPLETE - SCREEN SHOULD BE RED");
        Serial.println("========================================");

        while (true)
        {
            delay(1000);
        }
    }

    // ========================================================
    // Touch disabled
    // ========================================================

    void updateTouch() override
    {
    }

    bool isScreenTouched() override
    {
        return false;
    }

    int getTouchX() override
    {
        return 0;
    }

    int getTouchY() override
    {
        return 0;
    }

    // ========================================================
    // Communication disabled
    // ========================================================

    void sendProtocolData(
        uint8_t* data,
        uint16_t len
    ) override
    {
        (void)data;
        (void)len;
    }

    void sendCommand(
        const char* cmdString
    ) override
    {
        (void)cmdString;
    }

    void updateCommEngine() override
    {
    }

    CommState getCommState() override
    {
        return COMM_IDLE;
    }

    String getLastResponse() override
    {
        return "";
    }

    void clearCommState() override
    {
    }

    // ========================================================
    // Service Box rendering disabled
    // ========================================================

    void renderStartPage(
        bool forceRedraw,
        const char* statusMsg,
        uint16_t statusColor
    ) override
    {
        (void)forceRedraw;
        (void)statusMsg;
        (void)statusColor;
    }
};

// ============================================================
// Global instance
// ============================================================

HardwareWaveshare waveshareInstance;

HardwareInterface& Hardware = waveshareInstance;

#endif // SERVICEBOX_WAVESHARE