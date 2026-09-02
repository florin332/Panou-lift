#include "HardwareInterface.h"

#include <Arduino.h>
#include <SPI.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#define BAT_EN 26

#ifdef SERVICEBOX_WAVESHARE

// ============================================================
// WAVESHARE RP2350 TOUCH LCD 2.8"
// MINIMAL LCD TEST
//
// ST7789T3
// SPI1
//
// Transfer method:
//     Pico SDK spi_write_blocking()
//
// Based directly on the known-working standalone test
// from wv_2350_lcd commit ba3bfa4.
// ============================================================


// ============================================================
// LCD geometry
//
// IMPORTANT:
// The working standalone test uses:
//
//     LCD_WIDTH  = 240
//     LCD_HEIGHT = 320
//
// Total framebuffer:
//     240 * 320 * 2 = 153600 bytes
//
// ============================================================

#define LCD_WIDTH  240
#define LCD_HEIGHT 320


// ============================================================
// Low-level LCD command
//
// Equivalent to standalone test:
//
//     gpio_put(CS, 0);
//     gpio_put(DC, 0);
//     spi_write_blocking(spi1, &cmd, 1);
//     gpio_put(CS, 1);
// ============================================================

static void lcdWriteCommand(uint8_t cmd)
{
    gpio_put(TFT_CS, 0);
    gpio_put(TFT_DC, 0);

    spi_write_blocking(
        spi1,
        &cmd,
        1
    );

    gpio_put(TFT_CS, 1);
}


// ============================================================
// Low-level LCD data byte
// ============================================================

static void lcdWriteData(uint8_t data)
{
    gpio_put(TFT_CS, 0);
    gpio_put(TFT_DC, 1);

    spi_write_blocking(
        spi1,
        &data,
        1
    );

    gpio_put(TFT_CS, 1);
}


// ============================================================
// Low-level LCD data buffer
//
// CS remains LOW and DC remains HIGH for the ENTIRE transfer.
//
// This is the important difference from the previous
// HardwareWaveshare implementation.
// ============================================================

static void lcdWriteDataBuffer(
    const uint8_t* data,
    size_t length
)
{
    gpio_put(TFT_CS, 0);
    gpio_put(TFT_DC, 1);

    spi_write_blocking(
        spi1,
        data,
        length
    );

    gpio_put(TFT_CS, 1);
}


// ============================================================
// LCD hardware reset
//
// Exact sequence from the known-working standalone test.
// ============================================================

static void lcdReset()
{
    gpio_put(TFT_RST, 0);
    delay(50);

    gpio_put(TFT_RST, 1);
    delay(50);
}


// ============================================================
// ST7789 register initialization
//
// Exact register sequence from the working standalone test.
// ============================================================

static void lcdRegisterInit()
{
    // Display ON
    lcdWriteCommand(0x29);
    delay(10);

    // Sleep OUT
    lcdWriteCommand(0x11);
    delay(10);

    // Memory Access Control
    lcdWriteCommand(0x36);
    lcdWriteData(0x60);

    // Pixel Format: RGB565
    lcdWriteCommand(0x3A);
    lcdWriteData(0x05);

    // B0
    lcdWriteCommand(0xB0);
    lcdWriteData(0x00);
    lcdWriteData(0xE8);

    // Porch Setting
    lcdWriteCommand(0xB2);
    lcdWriteData(0x0C);
    lcdWriteData(0x0C);
    lcdWriteData(0x00);
    lcdWriteData(0x33);
    lcdWriteData(0x33);

    // Gate Control
    lcdWriteCommand(0xB7);
    lcdWriteData(0x75);

    // VCOM Setting
    lcdWriteCommand(0xBB);
    lcdWriteData(0x1A);

    // LCM Control
    lcdWriteCommand(0xC0);
    lcdWriteData(0x2C);

    // VDV and VRH Command Enable
    lcdWriteCommand(0xC2);
    lcdWriteData(0x01);
    lcdWriteData(0xFF);

    // VRHS
    lcdWriteCommand(0xC3);
    lcdWriteData(0x13);

    // VDV Set
    lcdWriteCommand(0xC4);
    lcdWriteData(0x20);

    // Frame Rate Control
    lcdWriteCommand(0xC6);
    lcdWriteData(0x0F);

    // Power Control 1
    lcdWriteCommand(0xD0);
    lcdWriteData(0xA4);
    lcdWriteData(0xA1);

    // D6
    lcdWriteCommand(0xD6);
    lcdWriteData(0xA1);

    // Positive Gamma
    lcdWriteCommand(0xE0);

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
    lcdWriteCommand(0xE1);

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
    lcdWriteCommand(0x21);

    // Display ON
    lcdWriteCommand(0x29);

    // Memory Write
    lcdWriteCommand(0x2C);
}


// ============================================================
// Rotation
//
// Exact logic from standalone test.
//
// rotation 0:
//     MADCTL = 0x00
// ============================================================

static void lcdSetRotation(uint16_t rotation)
{
    uint8_t data;

    lcdWriteCommand(0x36);

    switch (rotation)
    {
        case 1:
            data = 0x60;
            break;

        case 2:
            data = 0xC0;
            break;

        case 3:
            data = 0xA0;
            break;

        default:
            data = 0x00;
            break;
    }

    lcdWriteData(data);
}


// ============================================================
// LCD address window
//
// IMPORTANT:
//
// The working standalone test sends:
//
//     0 ... LCD_WIDTH  - 1
//     0 ... LCD_HEIGHT - 1
//
// directly as the four 16-bit coordinates.
//
// No Xend-1/Yend-1 is applied here because the caller already
// supplies inclusive endpoints.
// ============================================================

static void lcdSetWindow(
    uint16_t xStart,
    uint16_t yStart,
    uint16_t xEnd,
    uint16_t yEnd
)
{
    // Column Address Set
    lcdWriteCommand(0x2A);

    lcdWriteData(xStart >> 8);
    lcdWriteData(xStart & 0xFF);

    lcdWriteData(xEnd >> 8);
    lcdWriteData(xEnd & 0xFF);

    // Row Address Set
    lcdWriteCommand(0x2B);

    lcdWriteData(yStart >> 8);
    lcdWriteData(yStart & 0xFF);

    lcdWriteData(yEnd >> 8);
    lcdWriteData(yEnd & 0xFF);

    // Memory Write
    lcdWriteCommand(0x2C);
}


// ============================================================
// Fill entire LCD
//
// THIS IS THE CRITICAL PART.
//
// The working standalone test creates:
//
//     240 * 320 * 2 = 153600 bytes
//
// and sends the COMPLETE framebuffer with ONE
// spi_write_blocking() call.
//
// CS stays LOW for the entire framebuffer.
// DC stays HIGH for the entire framebuffer.
// ============================================================

static void lcdFill(uint16_t color)
{
    static uint8_t framebuffer[
        LCD_WIDTH * LCD_HEIGHT * 2
    ];

    const uint8_t hi = color >> 8;
    const uint8_t lo = color & 0xFF;

    // Build RGB565 framebuffer.
    //
    // MSB, LSB, MSB, LSB...
    //
    for (
        uint32_t i = 0;
        i < (uint32_t)LCD_WIDTH * LCD_HEIGHT;
        ++i
    )
    {
        framebuffer[i * 2]     = hi;
        framebuffer[i * 2 + 1] = lo;
    }

    // Full-screen window.
    lcdSetWindow(
        0,
        0,
        LCD_WIDTH - 1,
        LCD_HEIGHT - 1
    );

    // ONE continuous SPI transfer.
    lcdWriteDataBuffer(
        framebuffer,
        sizeof(framebuffer)
    );
}


// ============================================================
// Hardware implementation
// ============================================================

class HardwareWaveshare : public HardwareInterface
{
public:

    // ========================================================
    // Initialization
    // ========================================================

    void init() override
    {
        Serial.begin(115200);

        // Give time to open USB serial monitor.
        delay(10000);

        Serial.println();
        Serial.println("========================================");
        Serial.println(" WAVESHARE LCD TEST GREEN");
        Serial.println(" ST7789T3 / SPI1");
        Serial.println(" Pico SDK spi_write_blocking()");
        Serial.println("========================================");

        // ----------------------------------------------------
        // GPIO
        // ----------------------------------------------------

        pinMode(TFT_RST, OUTPUT);
        pinMode(TFT_DC,  OUTPUT);
        pinMode(TFT_CS,  OUTPUT);
        pinMode(TFT_BL,  OUTPUT);
        pinMode(BAT_EN,  OUTPUT);

        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TFT_DC, LOW);

        // Board power enable.
        digitalWrite(BAT_EN, HIGH);

        // Backlight.
        digitalWrite(TFT_BL, HIGH);

        Serial.println("[GPIO] OK");
        Serial.println("[BL] ON");

        // ----------------------------------------------------
        // SPI1
        //
        // Exact configuration validated by the standalone
        // working LCD test:
        //
        //     SPI1
        //     80 MHz
        //     Mode 3
        //     MSB first
        //
        // CS is controlled manually as GPIO.
        // ----------------------------------------------------

        spi_init(
            spi1,
            80 * 1000 * 1000
        );

        gpio_set_function(
            TFT_MOSI,
            GPIO_FUNC_SPI
        );

        gpio_set_function(
            TFT_SCLK,
            GPIO_FUNC_SPI
        );

        spi_set_format(
            spi1,
            8,
            SPI_CPOL_1,
            SPI_CPHA_1,
            SPI_MSB_FIRST
        );

        Serial.println(
            "[SPI1] OK @ 80 MHz / MODE3"
        );

        // ----------------------------------------------------
        // LCD initialization
        // ----------------------------------------------------

        lcdReset();

        lcdRegisterInit();

        // Exact standalone test behavior.
        lcdSetRotation(0);

        Serial.println(
            "[LCD] Initialization complete"
        );

        // ----------------------------------------------------
        // GREEN TEST
        // ----------------------------------------------------

        Serial.println(
            "WAVESHARE LCD TEST GREEN"
        );

        lcdFill(0x07E0);

        Serial.println(
            "FILL GREEN DONE"
        );

        // ----------------------------------------------------
        // Stay here.
        // ----------------------------------------------------

        while (true)
        {
            Serial.println(
                "FILL GREEN DONE"
            );

            delay(1000);
        }
    }


    // ========================================================
    // Touch
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
    // Communication
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
    // Rendering
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