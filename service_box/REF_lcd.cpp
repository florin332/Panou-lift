#include <Arduino.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

// ============================================================
// Waveshare RP2350 Touch LCD
// ST7789T3 - independent LCD test
//
// Transfer method copied from Cdemo2350:
//     spi_write_blocking(spi1, data, len)
//
// Arduino / Earle Philhower
// ============================================================

// ---------------- LCD pins ----------------

#define LCD_SCLK   10
#define LCD_MOSI   11
#define LCD_CS     13
#define LCD_DC     14
#define LCD_RST    15
#define LCD_BL     16

// ---------------- LCD geometry ----------------

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// ============================================================
// Low-level command
//
// Direct equivalent of:
//
// gpio_put(CS, 0);
// gpio_put(DC, 0);
// spi_write_blocking(spi1, &cmd, 1);
// gpio_put(CS, 1);
// ============================================================

static void lcdWriteCommand(uint8_t cmd)
{
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 0);

    spi_write_blocking(spi1, &cmd, 1);

    gpio_put(LCD_CS, 1);
}

// ============================================================
// Low-level data byte
//
// Direct equivalent of:
//
// gpio_put(CS, 0);
// gpio_put(DC, 1);
// spi_write_blocking(spi1, &data, 1);
// gpio_put(CS, 1);
// ============================================================

static void lcdWriteData(uint8_t data)
{
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 1);

    spi_write_blocking(spi1, &data, 1);

    gpio_put(LCD_CS, 1);
}

// ============================================================
// Low-level data buffer
//
// DIRECT equivalent of Cdemo2350:
//
// bsp_st7789_spi_write_data()
// ============================================================

static void lcdWriteDataBuffer(
    const uint8_t *data,
    size_t length
)
{
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 1);

    spi_write_blocking(
        spi1,
        data,
        length
    );

    gpio_put(LCD_CS, 1);
}

// ============================================================
// Hardware reset
//
// Exact Cdemo2350 timing
// ============================================================

static void lcdReset()
{
    gpio_put(LCD_RST, 0);
    delay(50);

    gpio_put(LCD_RST, 1);
    delay(50);
}

// ============================================================
// ST7789 register initialization
//
// Exact sequence from bsp_st7789_reg_init()
// ============================================================

static void lcdRegisterInit()
{
    lcdWriteCommand(0x29);
    delay(10);

    lcdWriteCommand(0x11);
    delay(10);

    lcdWriteCommand(0x36);
    lcdWriteData(0x60);

    lcdWriteCommand(0x3A);
    lcdWriteData(0x05);

    lcdWriteCommand(0xB0);
    lcdWriteData(0x00);
    lcdWriteData(0xE8);

    lcdWriteCommand(0xB2);
    lcdWriteData(0x0C);
    lcdWriteData(0x0C);
    lcdWriteData(0x00);
    lcdWriteData(0x33);
    lcdWriteData(0x33);

    lcdWriteCommand(0xB7);
    lcdWriteData(0x75);

    lcdWriteCommand(0xBB);
    lcdWriteData(0x1A);

    lcdWriteCommand(0xC0);
    lcdWriteData(0x2C);

    lcdWriteCommand(0xC2);
    lcdWriteData(0x01);
    lcdWriteData(0xFF);

    lcdWriteCommand(0xC3);
    lcdWriteData(0x13);

    lcdWriteCommand(0xC4);
    lcdWriteData(0x20);

    lcdWriteCommand(0xC6);
    lcdWriteData(0x0F);

    lcdWriteCommand(0xD0);
    lcdWriteData(0xA4);
    lcdWriteData(0xA1);

    lcdWriteCommand(0xD6);
    lcdWriteData(0xA1);

    // Positive gamma
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

    // Negative gamma
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

    lcdWriteCommand(0x21);
    lcdWriteCommand(0x29);
    lcdWriteCommand(0x2C);
}

// ============================================================
// Rotation
//
// Cdemo2350 calls set_rotation() after register init.
// Header uses rotation 0.
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
// Set window
//
// Exact logical sequence from bsp_st7789_set_window()
// offsets = 0
// ============================================================

static void lcdSetWindow(
    uint16_t xStart,
    uint16_t yStart,
    uint16_t xEnd,
    uint16_t yEnd
)
{
    // 0x2A - Column Address Set
    lcdWriteCommand(0x2A);

    lcdWriteData(xStart >> 8);
    lcdWriteData(xStart & 0xFF);

    lcdWriteData(xEnd >> 8);
    lcdWriteData(xEnd & 0xFF);

    // 0x2B - Row Address Set
    lcdWriteCommand(0x2B);

    lcdWriteData(yStart >> 8);
    lcdWriteData(yStart & 0xFF);

    lcdWriteData(yEnd >> 8);
    lcdWriteData(yEnd & 0xFF);

    // 0x2C - Memory Write
    lcdWriteCommand(0x2C);
}

// ============================================================
// Fill screen
//
// Exact equivalent of:
//
// bsp_st7789_flush(
//     0,
//     0,
//     width - 1,
//     height - 1,
//     color
// );
//
// using:
//
// bsp_st7789_spi_write_data(
//     (uint8_t *)color,
//     trans_count
// );
//
// We create a full RGB565 framebuffer and send it through
// spi_write_blocking() in ONE continuous transfer.
// ============================================================

static void lcdFill(uint16_t color)
{
    // 320 * 240 * 2 = 153600 bytes
    static uint8_t framebuffer[LCD_WIDTH * LCD_HEIGHT * 2];

    const uint8_t hi = color >> 8;
    const uint8_t lo = color & 0xFF;

    // Build framebuffer exactly as RGB565 byte stream:
    // MSB, LSB, MSB, LSB...
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
    {
        framebuffer[i * 2]     = hi;
        framebuffer[i * 2 + 1] = lo;
    }

    lcdSetWindow(
        0,
        0,
        LCD_WIDTH - 1,
        LCD_HEIGHT - 1
    );

    // This is the important part:
    //
    // Direct Pico SDK SPI transfer.
    //
    // CS LOW + DC HIGH for the ENTIRE framebuffer.
    lcdWriteDataBuffer(
        framebuffer,
        sizeof(framebuffer)
    );
}

// ============================================================
// LCD initialization
// ============================================================

static void lcdInit()
{
    // GPIO
    gpio_init(LCD_CS);
    gpio_init(LCD_DC);
    gpio_init(LCD_RST);

    gpio_set_dir(LCD_CS, GPIO_OUT);
    gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_set_dir(LCD_RST, GPIO_OUT);

    gpio_put(LCD_CS, 1);
    gpio_put(LCD_DC, 1);

    // Backlight
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    // --------------------------------------------------------
    // Exact Cdemo2350 SPI initialization
    // --------------------------------------------------------

    spi_init(
        spi1,
        80 * 1000 * 1000
    );

    gpio_set_function(
        LCD_MOSI,
        GPIO_FUNC_SPI
    );

    gpio_set_function(
        LCD_SCLK,
        GPIO_FUNC_SPI
    );

    spi_set_format(
        spi1,
        8,
        SPI_CPOL_1,
        SPI_CPHA_1,
        SPI_MSB_FIRST
    );

    // Reset
    lcdReset();

    // Registers
    lcdRegisterInit();

    // Cdemo2350:
    // bsp_st7789_set_rotation(st7789_info->rotation);
    //
    // BSP_ST7789_ROTATION = 0
    lcdSetRotation(0);
}

// ============================================================
// Arduino setup
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" Waveshare RP2350 LCD TEST");
    Serial.println(" ST7789T3 / SPI1");
    Serial.println(" Pico SDK spi_write_blocking()");
    Serial.println(" Based directly on Cdemo2350");
    Serial.println("========================================");

    Serial.println("SCLK = GP10");
    Serial.println("MOSI = GP11");
    Serial.println("CS   = GP13");
    Serial.println("DC   = GP14");
    Serial.println("RST  = GP15");
    Serial.println("BL   = GP16");

    Serial.println("SPI  = 80 MHz");
    Serial.println("MODE = 3");
    Serial.println("LCD  = 320 x 240");
    Serial.println("RGB565");

    lcdInit();

    Serial.println("LCD initialized.");
}

// ============================================================
// Test
// ============================================================

void loop()
{
    Serial.println("RED");

    lcdFill(0xF800);

    delay(1500);

    Serial.println("GREEN");

    lcdFill(0x07E0);

    delay(1500);

    Serial.println("BLUE");

    lcdFill(0x001F);

    delay(1500);

    Serial.println("WHITE");

    lcdFill(0xFFFF);

    delay(1500);

    Serial.println("BLACK");

    lcdFill(0x0000);

    delay(1500);
}