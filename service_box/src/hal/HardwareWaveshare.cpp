#include "HardwareInterface.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#ifdef SERVICEBOX_WAVESHARE

// ============================================================================
// WAVESHARE RP2350
// ============================================================================

// Service UART = UART1
#undef DISPLAY_SERIAL
#define DISPLAY_SERIAL Serial1

#define WAVESHARE_UART_TX 4
#define WAVESHARE_UART_RX 5

// LCD physical pins
#define WAVESHARE_LCD_SCLK 10
#define WAVESHARE_LCD_MOSI 11
#define WAVESHARE_LCD_CS   13
#define WAVESHARE_LCD_DC   14
#define WAVESHARE_LCD_RST  15
#define WAVESHARE_LCD_BL   16

// Native LCD geometry
// IMPORTANT: ST7789 native addressing for this board is 240x320.
// Landscape rotation is handled later by the display layer if required.
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// ============================================================================
// Minimal font interface
// ============================================================================

typedef struct {
    const uint8_t *table;
    uint16_t Width;
    uint16_t Height;
} UT_Font;

extern const UT_Font Font24;
extern const UT_Font Font16;
extern const UT_Font Font12;

// ============================================================================
// Hardware implementation
// ============================================================================

class HardwareWaveshare : public HardwareInterface {
private:

    int _touchX = 0;
    int _touchY = 0;
    bool _isTouched = false;

    char _lastStatus[32] = "";
    uint16_t _lastStatusColor = 0x0000;

    // CST328
    static constexpr uint8_t CST328_ADDR = 0x1A;
    static constexpr uint16_t CST328_REG_TOUCH_INFO = 0xD000;

    // ------------------------------------------------------------------------
    // LCD low-level write
    // ------------------------------------------------------------------------

    void lcdWriteCmd(uint8_t cmd)
    {
        digitalWrite(WAVESHARE_LCD_DC, LOW);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        spi_write_blocking(spi1, &cmd, 1);

        digitalWrite(WAVESHARE_LCD_CS, HIGH);
    }

    void lcdWriteData(uint8_t data)
    {
        digitalWrite(WAVESHARE_LCD_DC, HIGH);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        spi_write_blocking(spi1, &data, 1);

        digitalWrite(WAVESHARE_LCD_CS, HIGH);
    }

    void lcdWriteDataBuffer(const uint8_t *data, size_t len)
    {
        if (data == nullptr || len == 0)
            return;

        digitalWrite(WAVESHARE_LCD_DC, HIGH);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        spi_write_blocking(spi1, data, len);

        digitalWrite(WAVESHARE_LCD_CS, HIGH);
    }

    // ------------------------------------------------------------------------
    // ST7789 initialization
    // Exact sequence based on the working Waveshare/Cdemo BSP.
    // ------------------------------------------------------------------------

    void initWaveshareRegisters()
    {
        DISPLAY_SERIAL.println("[LCD] ST7789 reset...");

        digitalWrite(WAVESHARE_LCD_RST, HIGH);
        delay(100);

        digitalWrite(WAVESHARE_LCD_RST, LOW);
        delay(100);

        digitalWrite(WAVESHARE_LCD_RST, HIGH);
        delay(100);

        DISPLAY_SERIAL.println("[LCD] ST7789 register initialization...");

        lcdWriteCmd(0x29);
        delay(10);

        lcdWriteCmd(0x11);
        delay(10);

        lcdWriteCmd(0x36);
        lcdWriteData(0x00);

        lcdWriteCmd(0x3A);
        lcdWriteData(0x05);

        lcdWriteCmd(0xB0);
        lcdWriteData(0x00);
        lcdWriteData(0xE8);

        lcdWriteCmd(0xB2);
        lcdWriteData(0x0C);
        lcdWriteData(0x0C);
        lcdWriteData(0x00);
        lcdWriteData(0x33);
        lcdWriteData(0x33);

        lcdWriteCmd(0xB7);
        lcdWriteData(0x75);

        lcdWriteCmd(0xBB);
        lcdWriteData(0x1A);

        lcdWriteCmd(0xC0);
        lcdWriteData(0x2C);

        lcdWriteCmd(0xC2);
        lcdWriteData(0x01);
        lcdWriteData(0xFF);

        lcdWriteCmd(0xC3);
        lcdWriteData(0x13);

        lcdWriteCmd(0xC4);
        lcdWriteData(0x20);

        lcdWriteCmd(0xC6);
        lcdWriteData(0x0F);

        lcdWriteCmd(0xD0);
        lcdWriteData(0xA4);
        lcdWriteData(0xA1);

        lcdWriteCmd(0xD6);
        lcdWriteData(0xA1);

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

        lcdWriteCmd(0x21);
        lcdWriteCmd(0x29);
        lcdWriteCmd(0x2C);

        DISPLAY_SERIAL.println("[LCD] ST7789 initialization complete");
    }

    // ------------------------------------------------------------------------
    // ST7789 address window
    //
    // xEnd/yEnd are INCLUSIVE.
    // This matches the original BSP:
    //
    //   0..239
    //   0..319
    // ------------------------------------------------------------------------

    void lcdSetWindow(
        uint16_t xStart,
        uint16_t yStart,
        uint16_t xEnd,
        uint16_t yEnd
    )
    {
        uint8_t data[4];

        if (xStart >= LCD_WIDTH)
            xStart = LCD_WIDTH - 1;

        if (xEnd >= LCD_WIDTH)
            xEnd = LCD_WIDTH - 1;

        if (yStart >= LCD_HEIGHT)
            yStart = LCD_HEIGHT - 1;

        if (yEnd >= LCD_HEIGHT)
            yEnd = LCD_HEIGHT - 1;

        // Column address
        lcdWriteCmd(0x2A);

        data[0] = static_cast<uint8_t>(xStart >> 8);
        data[1] = static_cast<uint8_t>(xStart & 0xFF);
        data[2] = static_cast<uint8_t>(xEnd >> 8);
        data[3] = static_cast<uint8_t>(xEnd & 0xFF);

        lcdWriteDataBuffer(data, 4);

        // Row address
        lcdWriteCmd(0x2B);

        data[0] = static_cast<uint8_t>(yStart >> 8);
        data[1] = static_cast<uint8_t>(yStart & 0xFF);
        data[2] = static_cast<uint8_t>(yEnd >> 8);
        data[3] = static_cast<uint8_t>(yEnd & 0xFF);

        lcdWriteDataBuffer(data, 4);

        // Memory write
        lcdWriteCmd(0x2C);
    }

    // ------------------------------------------------------------------------
    // Fill complete screen
    // ------------------------------------------------------------------------

    void clearCanvasColor(uint16_t color)
    {
        lcdSetWindow(
            0,
            0,
            LCD_WIDTH - 1,
            LCD_HEIGHT - 1
        );

        // One complete RGB565 scanline.
        static uint8_t line[LCD_WIDTH * 2];

        const uint8_t hi = static_cast<uint8_t>(color >> 8);
        const uint8_t lo = static_cast<uint8_t>(color & 0xFF);

        for (uint16_t x = 0; x < LCD_WIDTH; ++x) {
            line[x * 2]     = hi;
            line[x * 2 + 1] = lo;
        }

        digitalWrite(WAVESHARE_LCD_DC, HIGH);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        for (uint16_t y = 0; y < LCD_HEIGHT; ++y) {
            spi_write_blocking(
                spi1,
                line,
                sizeof(line)
            );
        }

        digitalWrite(WAVESHARE_LCD_CS, HIGH);
    }

    // ------------------------------------------------------------------------
    // Simple status character
    // ------------------------------------------------------------------------

    void drawStatusBarChar(
        int x,
        int y,
        char c,
        uint16_t color
    )
    {
        if (c == ' ')
            return;

        // Keep drawing inside the physical screen.
        if (x < 0 || y < 0)
            return;

        if (x + 7 >= LCD_WIDTH || y + 15 >= LCD_HEIGHT)
            return;

        lcdSetWindow(
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
            static_cast<uint16_t>(x + 7),
            static_cast<uint16_t>(y + 15)
        );

        static uint8_t charLine[8 * 2];

        const uint8_t hi = static_cast<uint8_t>(color >> 8);
        const uint8_t lo = static_cast<uint8_t>(color & 0xFF);

        for (int i = 0; i < 8; ++i) {
            charLine[i * 2]     = hi;
            charLine[i * 2 + 1] = lo;
        }

        digitalWrite(WAVESHARE_LCD_DC, HIGH);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        for (int row = 0; row < 16; ++row) {
            spi_write_blocking(
                spi1,
                charLine,
                sizeof(charLine)
            );
        }

        digitalWrite(WAVESHARE_LCD_CS, HIGH);
    }

    // ------------------------------------------------------------------------
    // Status text
    // ------------------------------------------------------------------------

    void drawStatusText(
        const char *text,
        uint16_t color
    )
    {
        if (text == nullptr)
            return;

        int len = strlen(text);

        if (len > 20)
            len = 20;

        const int charW = 8;
        const int charH = 16;
        const int spacing = 2;

        const int totalW = len * (charW + spacing);
        const int xStart = (LCD_WIDTH - totalW) / 2;
        const int yStart = 250;

        // Clear previous status strip.
        lcdSetWindow(
            10,
            yStart,
            229,
            yStart + charH - 1
        );

        static uint8_t blackLine[220 * 2];

        memset(blackLine, 0, sizeof(blackLine));

        digitalWrite(WAVESHARE_LCD_DC, HIGH);
        digitalWrite(WAVESHARE_LCD_CS, LOW);

        for (int row = 0; row < charH; ++row) {
            spi_write_blocking(
                spi1,
                blackLine,
                sizeof(blackLine)
            );
        }

        digitalWrite(WAVESHARE_LCD_CS, HIGH);

        // Draw simple status blocks.
        for (int i = 0; i < len; ++i) {
            drawStatusBarChar(
                xStart + i * (charW + spacing),
                yStart,
                text[i],
                color
            );
        }
    }

public:

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void init() override
    {
        // --------------------------------------------------------------------
        // UART
        // --------------------------------------------------------------------

        DISPLAY_SERIAL.setTX(WAVESHARE_UART_TX);
        DISPLAY_SERIAL.setRX(WAVESHARE_UART_RX);
        DISPLAY_SERIAL.begin(115200);

        // --------------------------------------------------------------------
        // Waveshare board power
        // --------------------------------------------------------------------

        pinMode(26, OUTPUT);
        digitalWrite(26, HIGH);

        // --------------------------------------------------------------------
        // Touch I2C
        // --------------------------------------------------------------------

        Wire.setSDA(TOUCH_SDA);
        Wire.setSCL(TOUCH_SCL);
        Wire.begin();
        Wire.setClock(100000);

        // CST328 reset
        pinMode(TOUCH_RST, OUTPUT);

        digitalWrite(TOUCH_RST, HIGH);
        delay(10);

        digitalWrite(TOUCH_RST, LOW);
        delay(20);

        digitalWrite(TOUCH_RST, HIGH);
        delay(20);

        pinMode(TOUCH_INT, INPUT_PULLUP);

        // --------------------------------------------------------------------
        // LCD GPIO
        // --------------------------------------------------------------------

        pinMode(WAVESHARE_LCD_CS, OUTPUT);
        digitalWrite(WAVESHARE_LCD_CS, HIGH);

        pinMode(WAVESHARE_LCD_DC, OUTPUT);
        digitalWrite(WAVESHARE_LCD_DC, LOW);

        pinMode(WAVESHARE_LCD_RST, OUTPUT);
        digitalWrite(WAVESHARE_LCD_RST, HIGH);

        pinMode(WAVESHARE_LCD_BL, OUTPUT);
        digitalWrite(WAVESHARE_LCD_BL, LOW);

        // --------------------------------------------------------------------
        // SD CS
        //
        // SD shares the physical SPI bus.
        // Keep SD deselected.
        // --------------------------------------------------------------------

        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);

        // --------------------------------------------------------------------
        // SPI1
        //
        // Exact configuration validated with the standalone LCD test:
        //
        //   SPI1
        //   80 MHz
        //   Mode 3
        //   MSB first
        // --------------------------------------------------------------------

        spi_init(
            spi1,
            80 * 1000 * 1000
        );

        gpio_set_function(
            WAVESHARE_LCD_MOSI,
            GPIO_FUNC_SPI
        );

        gpio_set_function(
            WAVESHARE_LCD_SCLK,
            GPIO_FUNC_SPI
        );

        spi_set_format(
            spi1,
            8,
            SPI_CPOL_1,
            SPI_CPHA_1,
            SPI_MSB_FIRST
        );

        DISPLAY_SERIAL.println(
            "[INIT] SPI1 = 80MHz / MODE3 / MSB"
        );

        // --------------------------------------------------------------------
        // ST7789
        // --------------------------------------------------------------------

        initWaveshareRegisters();

        // --------------------------------------------------------------------
        // Initial screen
        // --------------------------------------------------------------------

        DISPLAY_SERIAL.println(
            "[INIT] Clearing LCD..."
        );

        clearCanvasColor(0x0000);

        delay(50);

        // GP16 is the confirmed physical backlight pin.
        digitalWrite(
            WAVESHARE_LCD_BL,
            HIGH
        );

        DISPLAY_SERIAL.println(
            "[INIT] Backlight ON"
        );

        DISPLAY_SERIAL.println(
            "[INIT] Waveshare hardware initialization complete"
        );
    }

    // ========================================================================
    // TOUCH
    // ========================================================================

    void updateTouch() override
    {
        if (digitalRead(TOUCH_INT) == HIGH) {
            _isTouched = false;
            return;
        }

        Wire.beginTransmission(CST328_ADDR);

        Wire.write(
            static_cast<uint8_t>(
                CST328_REG_TOUCH_INFO >> 8
            )
        );

        Wire.write(
            static_cast<uint8_t>(
                CST328_REG_TOUCH_INFO & 0xFF
            )
        );

        if (Wire.endTransmission(false) != 0) {
            _isTouched = false;
            return;
        }

        constexpr uint8_t requested = 5;

        uint8_t received =
            Wire.requestFrom(
                CST328_ADDR,
                requested
            );

        if (received < requested) {
            _isTouched = false;
            return;
        }

        uint8_t idState = Wire.read();
        uint8_t xHigh   = Wire.read();
        uint8_t yHigh   = Wire.read();
        uint8_t xyLow   = Wire.read();

        (void)Wire.read();

        bool pressed =
            ((idState & 0x0F) == 0x06);

        if (!pressed) {
            _isTouched = false;
            return;
        }

        uint16_t rawX =
            (static_cast<uint16_t>(xHigh) << 4) |
            (xyLow >> 4);

        uint16_t rawY =
            (static_cast<uint16_t>(yHigh) << 4) |
            (xyLow & 0x0F);

        // CST328 -> portrait 240x320
        int mappedX =
            239 -
            static_cast<int>(
                (rawY * 240UL) / 4096UL
            );

        int mappedY =
            static_cast<int>(
                (rawX * 320UL) / 4096UL
            );

        if (mappedX < 0)
            mappedX = 0;

        if (mappedX > 239)
            mappedX = 239;

        if (mappedY < 0)
            mappedY = 0;

        if (mappedY > 319)
            mappedY = 319;

        _touchX = mappedX;
        _touchY = mappedY;
        _isTouched = true;
    }

    bool isScreenTouched() override
    {
        return _isTouched;
    }

    int getTouchX() override
    {
        return _touchX;
    }

    int getTouchY() override
    {
        return _touchY;
    }

    // ========================================================================
    // COMMUNICATION
    // ========================================================================

    void sendProtocolData(
        uint8_t *data,
        uint16_t len
    ) override
    {
        DISPLAY_SERIAL.write(data, len);
    }

    void sendCommand(
        const char *cmdString
    ) override
    {
        DISPLAY_SERIAL.print(
            String(cmdString) + "\r\n"
        );
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

    // ========================================================================
    // RENDERING
    // ========================================================================

    void renderStartPage(
        bool forceRedraw,
        const char *statusMsg,
        uint16_t statusColor
    ) override
    {
        if (forceRedraw) {

            // Full portrait screen.
            clearCanvasColor(0x0000);

            // Test canvas / main UI area.
            lcdSetWindow(
                10,
                10,
                229,
                309
            );

            static uint8_t line[LCD_WIDTH * 2];

            const uint8_t hi = 0x19;
            const uint8_t lo = 0x67;

            for (uint16_t x = 0; x < LCD_WIDTH; ++x) {
                line[x * 2]     = hi;
                line[x * 2 + 1] = lo;
            }

            digitalWrite(WAVESHARE_LCD_DC, HIGH);
            digitalWrite(WAVESHARE_LCD_CS, LOW);

            for (uint16_t y = 10; y <= 309; ++y) {

                // Only 220 pixels are inside the window.
                spi_write_blocking(
                    spi1,
                    line,
                    220 * 2
                );
            }

            digitalWrite(WAVESHARE_LCD_CS, HIGH);

            _lastStatus[0] = '\0';
            _lastStatusColor = 0x0000;
        }

        // Update status only when it changed.
        if (
            statusMsg != nullptr &&
            (
                strcmp(_lastStatus, statusMsg) != 0 ||
                _lastStatusColor != statusColor
            )
        ) {
            drawStatusText(
                statusMsg,
                statusColor
            );

            strncpy(
                _lastStatus,
                statusMsg,
                sizeof(_lastStatus) - 1
            );

            _lastStatus[
                sizeof(_lastStatus) - 1
            ] = '\0';

            _lastStatusColor = statusColor;
        }

        DISPLAY_SERIAL.print(
            "[Display Render Event Update] Status String: "
        );

        DISPLAY_SERIAL.println(
            statusMsg != nullptr
                ? statusMsg
                : ""
        );
    }
};

// ============================================================================
// Global hardware instance
// ============================================================================

HardwareWaveshare waveshareInstance;
HardwareInterface& Hardware = waveshareInstance;

#endif // SERVICEBOX_WAVESHARE