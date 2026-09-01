#include "HardwareInterface.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#ifdef SERVICEBOX_WAVESHARE

// Override the USB-CDC fallback from HardwareInterface.h for the Waveshare board.
// The service link is wired to UART1 (TX=GP4, RX=GP5).
#undef DISPLAY_SERIAL
#define DISPLAY_SERIAL Serial1

#define WAVESHARE_UART_TX 4
#define WAVESHARE_UART_RX 5

// Custom simple types to avoid redefining full bloated font structures
typedef struct {
    const uint8_t *table;
    uint16_t Width;
    uint16_t Height;
} UT_Font;

// Mapping internal external font links from background build units
extern const UT_Font Font24;
extern const UT_Font Font16;
extern const UT_Font Font12;

class HardwareWaveshare : public HardwareInterface {
private:
    int _touchX = 0;
    int _touchY = 0;
    bool _isTouched = false;

    // Last rendered status state to avoid clearing the whole screen every update
    char _lastStatus[32] = "";
    uint16_t _lastStatusColor = 0x0000;

    // CST328 I2C constants
    static constexpr uint8_t CST328_ADDR = 0x1A;
    static constexpr uint16_t CST328_REG_TOUCH_INFO = 0xD000;

    // Fast inline byte transmission for SPI register selection blocks
    void lcdWriteCmd(uint8_t cmd) {
        digitalWrite(TFT_DC, LOW);
        delayMicroseconds(10);
        digitalWrite(TFT_CS, LOW);
        delayMicroseconds(10);
        SPI1.transfer(cmd);
        delayMicroseconds(10);
        digitalWrite(TFT_CS, HIGH);
        delayMicroseconds(10);
    }

    void lcdWriteData(uint8_t data) {
        digitalWrite(TFT_DC, HIGH);
        delayMicroseconds(10);
        digitalWrite(TFT_CS, LOW);
        delayMicroseconds(10);
        SPI1.transfer(data);
        delayMicroseconds(10);
        digitalWrite(TFT_CS, HIGH);
        delayMicroseconds(10);
    }

    // Official native hardware register wakeup sequence sequence extracted from demo-rp2030
    void initWaveshareRegisters() {
        DISPLAY_SERIAL.println("[INIT] Starting ST7789T3 reset sequence...");
        
        digitalWrite(TFT_RST, HIGH);
        delay(20);
        digitalWrite(TFT_RST, LOW);
        delay(20);
        digitalWrite(TFT_RST, HIGH);
        delay(50);

        DISPLAY_SERIAL.println("[INIT] Reset complete, sending Sleep Out...");
        
        // Extended driver power tuning commands (Required to turn on ST7789T3 matrices)
        lcdWriteCmd(0x11); // Sleep Out sequence command
        delay(150);

        lcdWriteCmd(0x36); // Memory Data Access Control
        lcdWriteData(0x08); // Portrait orientation (MY=0, MX=0, MV=0, ML=1)

        lcdWriteCmd(0x3A); // Interface Pixel Format setup register
        lcdWriteData(0x05); // Standard 16-bit color selection (RGB565 matrix structure)

        // Gamma curves tuning tables matching vendor factory profile configuration
        lcdWriteCmd(0xB2); lcdWriteData(0x0C); lcdWriteData(0x0C); lcdWriteData(0x00); lcdWriteData(0x33); lcdWriteData(0x33);
        lcdWriteCmd(0xB7); lcdWriteData(0x35);
        lcdWriteCmd(0xBB); lcdWriteData(0x19);
        lcdWriteCmd(0xC0); lcdWriteData(0x2C);
        lcdWriteCmd(0xC2); lcdWriteData(0x01);
        lcdWriteCmd(0xC3); lcdWriteData(0x12);
        lcdWriteCmd(0xC4); lcdWriteData(0x20);
        lcdWriteCmd(0xC6); lcdWriteData(0x0F);
        lcdWriteCmd(0xD0); lcdWriteData(0xA4); lcdWriteData(0xA1);

        // Positive and Negative Gamma control arrays
        lcdWriteCmd(0xE0);
        lcdWriteData(0xD0); lcdWriteData(0x04); lcdWriteData(0x0D); lcdWriteData(0x11);
        lcdWriteData(0x13); lcdWriteData(0x2B); lcdWriteData(0x3F); lcdWriteData(0x54);
        lcdWriteData(0x4C); lcdWriteData(0x18); lcdWriteData(0x0D); lcdWriteData(0x0B);
        lcdWriteData(0x1F); lcdWriteData(0x23);

        lcdWriteCmd(0xE1);
        lcdWriteData(0xD0); lcdWriteData(0x04); lcdWriteData(0x0C); lcdWriteData(0x11);
        lcdWriteData(0x13); lcdWriteData(0x2C); lcdWriteData(0x3F); lcdWriteData(0x44);
        lcdWriteData(0x51); lcdWriteData(0x2F); lcdWriteData(0x1F); lcdWriteData(0x1F);
        lcdWriteData(0x20); lcdWriteData(0x23);

        lcdWriteCmd(0x21); // Display Inversion On configuration register (Waveshare standard match)
        delay(10);
        lcdWriteCmd(0x29); // Turn Display Main Gate ON
        delay(10);
        
        DISPLAY_SERIAL.println("[INIT] ST7789T3 initialization complete!");
    }

    void lcdSetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
        lcdWriteCmd(0x2A); // Column Address Set register pointer
        lcdWriteData(xStart >> 8); lcdWriteData(xStart & 0xFF);
        lcdWriteData((xEnd - 1) >> 8); lcdWriteData((xEnd - 1) & 0xFF);

        lcdWriteCmd(0x2B); // Row Address Set register pointer
        lcdWriteData(yStart >> 8); lcdWriteData(yStart & 0xFF);
        lcdWriteData((yEnd - 1) >> 8); lcdWriteData((yEnd - 1) & 0xFF);

        lcdWriteCmd(0x2C); // Memory Write start sequence token
    }

    void clearCanvasColor(uint16_t color) {
        lcdSetWindow(0, 0, 240, 320);
        delayMicroseconds(100);
        digitalWrite(TFT_DC, HIGH);
        delayMicroseconds(10);
        digitalWrite(TFT_CS, LOW);
        delayMicroseconds(10);
        
        // Fast streaming fill sequence loop avoiding local RAM buffers pressure bottlenecks
        for (uint32_t i = 0; i < 240 * 320; i++) {
            SPI1.transfer(color >> 8);
            SPI1.transfer(color & 0xFF);
        }
        
        delayMicroseconds(10);
        digitalWrite(TFT_CS, HIGH);
        delayMicroseconds(10);
    }

public:
    void init() override {
        // Service UART on GP4/GP5 (UART1)
        DISPLAY_SERIAL.setTX(WAVESHARE_UART_TX);
        DISPLAY_SERIAL.setRX(WAVESHARE_UART_RX);
        DISPLAY_SERIAL.begin(115200);

        // Touch controller I2C0 on GP6/GP7
        Wire.setSDA(TOUCH_SDA);
        Wire.setSCL(TOUCH_SCL);
        Wire.begin();
        Wire.setClock(100000);

        // CST328 hardware reset on GP16 (active-low), INT on GP17
        pinMode(TOUCH_RST, OUTPUT);
        digitalWrite(TOUCH_RST, HIGH);
        delay(10);
        digitalWrite(TOUCH_RST, LOW);
        delay(20);
        digitalWrite(TOUCH_RST, HIGH);
        delay(20);
        pinMode(TOUCH_INT, INPUT_PULLUP);

        // Set control pin configuration directions cleanly
        pinMode(TFT_CS, OUTPUT);
        digitalWrite(TFT_CS, HIGH);
        pinMode(TFT_DC, OUTPUT);
        pinMode(TFT_RST, OUTPUT);

        // SD card shares the same SPI1 bus as the LCD; keep it deselected
        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);

        #if defined(TFT_BL) && (TFT_BL >= 0)
            pinMode(TFT_BL, OUTPUT);
            digitalWrite(TFT_BL, LOW); // Hold off backlight to prevent white flash glitch patterns
        #endif

        // Run SPI1 bus core mapping (display/SD are wired to SPI1 pins: SCK=GP10, MOSI=GP11)
        SPI1.setTX(TFT_MOSI);
        SPI1.setSCK(TFT_SCLK);
        SPI1.begin();

        DISPLAY_SERIAL.println("[INIT] SPI1 initialized, calling display init...");
        
        // Fire native low-level startup routine sequence
        initWaveshareRegisters();
        
        DISPLAY_SERIAL.println("[INIT] Clearing canvas to black...");
        clearCanvasColor(0x0000); // Black

        DISPLAY_SERIAL.println("[INIT] Turning backlight ON...");
        // Stabilize charging lines and assert backlight to HIGH
        delay(50);
        #if defined(TFT_BL) && (TFT_BL >= 0)
            digitalWrite(TFT_BL, HIGH);
        #endif
        
        DISPLAY_SERIAL.println("[INIT] Hardware initialization complete!");
    }

    void updateTouch() override {
        // Poll CST328 touch controller. INT is active-low when a touch event is present.
        if (digitalRead(TOUCH_INT) == HIGH) {
            _isTouched = false;
            return;
        }

        // Read first finger record (5 bytes) from 0xD000
        Wire.beginTransmission(CST328_ADDR);
        Wire.write(static_cast<uint8_t>(CST328_REG_TOUCH_INFO >> 8));
        Wire.write(static_cast<uint8_t>(CST328_REG_TOUCH_INFO & 0xFF));
        if (Wire.endTransmission(false) != 0) {
            _isTouched = false;
            return;
        }

        uint8_t requested = 5;
        uint8_t received = Wire.requestFrom(CST328_ADDR, requested);
        if (received < requested) {
            _isTouched = false;
            return;
        }

        uint8_t idState = Wire.read();
        uint8_t xHigh    = Wire.read();
        uint8_t yHigh    = Wire.read();
        uint8_t xyLow    = Wire.read();
        (void)Wire.read(); // unused, read to advance buffer

        // State 0x06 means the first finger is pressed
        bool pressed = ((idState & 0x0F) == 0x06);
        if (!pressed) {
            _isTouched = false;
            return;
        }

        uint16_t rawX = (static_cast<uint16_t>(xHigh) << 4) | (xyLow >> 4);
        uint16_t rawY = (static_cast<uint16_t>(yHigh) << 4) | (xyLow & 0x0F);

        // CST328 native coordinates are ~0..4095. Map to 240x320 portrait panel.
        // Empirical orientation for the Waveshare 2.8" Touch LCD: swap and invert
        // so the USB edge is at the bottom.
        int mappedX = 239 - static_cast<int>((rawY * 240UL) / 4096UL);
        int mappedY = static_cast<int>((rawX * 320UL) / 4096UL);

        // Clamp
        if (mappedX < 0) mappedX = 0;
        if (mappedX > 239) mappedX = 239;
        if (mappedY < 0) mappedY = 0;
        if (mappedY > 319) mappedY = 319;

        _touchX = mappedX;
        _touchY = mappedY;
        _isTouched = true;
    }
    bool isScreenTouched() override { return _isTouched; }
    int getTouchX() override { return _touchX; }
    int getTouchY() override { return _touchY; }

    void sendProtocolData(uint8_t* data, uint16_t len) override { DISPLAY_SERIAL.write(data, len); }
    void sendCommand(const char* cmdString) override { DISPLAY_SERIAL.print(String(cmdString) + "\r\n"); }
    void updateCommEngine() override {}
    CommState getCommState() override { return COMM_IDLE; }
    String getLastResponse() override { return ""; }
    void clearCommState() override {}
    
    // ========================================================================
    // NATIVE DIRECT REGISTER RENDERING (PORTRAIT 240x320)
    // ========================================================================
    // Fast status text rendered as simple colored bars (8x16 monospace-ish).
    // This is only intended for status messages; full UI should use a real font.
    void drawStatusBarChar(int x, int y, char c, uint16_t color) {
        // Basic 8x16 block: draw filled rectangle if printable, skip if space
        if (c == ' ') return;
        lcdSetWindow(x, y, x + 7, y + 15);
        digitalWrite(TFT_DC, HIGH);
        digitalWrite(TFT_CS, LOW);
        for (int i = 0; i < 8 * 16; i++) {
            SPI1.transfer(color >> 8);
            SPI1.transfer(color & 0xFF);
        }
        digitalWrite(TFT_CS, HIGH);
    }

    void drawStatusText(const char* text, uint16_t color) {
        // Center a status message roughly in the lower third of the 240x320 screen
        int len = strlen(text);
        if (len > 20) len = 20;
        int charW = 8;
        int charH = 16;
        int totalW = len * (charW + 2);
        int xStart = (240 - totalW) / 2;
        int yStart = 250;

        // Clear previous background strip
        lcdSetWindow(10, yStart, 230, yStart + charH);
        digitalWrite(TFT_DC, HIGH);
        digitalWrite(TFT_CS, LOW);
        for (int i = 0; i < 220 * charH; i++) {
            SPI1.transfer(0x00); SPI1.transfer(0x00); // black
        }
        digitalWrite(TFT_CS, HIGH);

        for (int i = 0; i < len; i++) {
            drawStatusBarChar(xStart + i * (charW + 2), yStart, text[i], color);
        }
    }

    void renderStartPage(bool forceRedraw, const char* statusMsg, uint16_t statusColor) override {
        if (forceRedraw) {
            clearCanvasColor(0x0000); // Clear background canvas to black layout

            // Quick drawing checkpoints to trace compilation without full font loaders layers overhead
            // We will hook your font16/font24 arrays or render direct graphics blocks
            // For immediate testing, we clear layout and draw an outer frame box anchor
            lcdSetWindow(10, 10, 230, 310);
            digitalWrite(TFT_DC, HIGH);
            digitalWrite(TFT_CS, LOW);
            for(uint32_t i=0; i < 220*300; i++) {
                SPI1.transfer(0x19); // Draw deep blue base testing canvas structure
                SPI1.transfer(0x67);
            }
            digitalWrite(TFT_CS, HIGH);

            // Reset cached status so the next non-forced update always redraws text
            _lastStatus[0] = '\0';
            _lastStatusColor = 0x0000;
        }

        // Only redraw status text when it actually changes
        if (strcmp(_lastStatus, statusMsg) != 0 || _lastStatusColor != statusColor) {
            drawStatusText(statusMsg, statusColor);
            strncpy(_lastStatus, statusMsg, sizeof(_lastStatus) - 1);
            _lastStatus[sizeof(_lastStatus) - 1] = '\0';
            _lastStatusColor = statusColor;
        }

        // Output quick trace print message directly into PC serial terminal monitoring connection log
        DISPLAY_SERIAL.print("[Display Render Event Update] Status String: ");
        DISPLAY_SERIAL.println(statusMsg);
    }
};

HardwareWaveshare waveshareInstance;
HardwareInterface& Hardware = waveshareInstance;

#endif // SERVICEBOX_WAVESHARE
