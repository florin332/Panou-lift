// Drivers/Display.cpp (Motorul Grafic Complet - V10.29 + Service Overlay)

#include "Display.h"

// 1. Core definitions loaded via relative subfolder paths
#include "Pins.h"
#include "Config.h"
#include "BuildInfo.h"
#include "SharedPanel.h"
#include "Settings.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Custom_ST7735.h"
#include <SPI.h>

// 2. Custom hardware dashboard fonts loaded from src/Fonts/
#include "../Fonts/oneslot30.h"
#include "../Fonts/glyph40.h"
#include "../Fonts/arrows40.h"
#include "../Fonts/universalis12.h"
#include "../Fonts/oneslot65.h"
#include "../Fonts/tables40.h"

namespace Display
{
    // Dual hardware display instances bound via Pins layout mappings
    static Custom_ST7735 tft1(
        Pins::TFT1::CS,
        Pins::TFT1::DC,
        Pins::TFT1::RST);

    static Custom_ST7735 tft2(
        &SPI1,
        Pins::TFT2::CS,
        Pins::TFT2::DC,
        Pins::TFT2::RST);

    // Isolated internal color constants mapped for 16-bit 565 format execution
    constexpr uint16_t COLOR_BLACK  = 0x0000;
    constexpr uint16_t COLOR_WHITE  = 0xFFFF;
    constexpr uint16_t COLOR_GREY   = 0x7BEF;
    constexpr uint16_t COLOR_YELLOW = 0xFFE0;
    constexpr uint16_t COLOR_GREEN  = 0x07E0;
    constexpr uint16_t COLOR_ORANGE = 0xFD20;
    constexpr uint16_t COLOR_RED    = 0xF800;

    static unsigned long lastActivityMillis = 0;
    static bool backlightOn = true;
    static bool standbyActive = false;
    static bool serviceModeActive = false;

    static void setBacklight(bool enabled) {
        if (backlightOn == enabled) return;
        digitalWrite(Pins::UI::BACKLIGHT, enabled ? HIGH : LOW);
        backlightOn = enabled;
    }

    static void initializeController(Custom_ST7735 &tft) {
        tft.initR(INITR_BLACKTAB);
        tft.setRowStart(Config::Display::ROW_START);
        tft.setColStart(Config::Display::COL_START);
        tft.setRotation(Config::Display::ROTATION);
        tft.setSPISpeed(Config::Display::SPI_CLOCK);
    }

    static void initializeControllers() {
        initializeController(tft1);
        initializeController(tft2);
    }

    static void drawBootScreen(Adafruit_ST7735 &tft, const char *elevatorName) {
        char hardwareVersion[16];
        char firmwareVersion[20];
        snprintf(hardwareVersion, sizeof(hardwareVersion), "HARDWARE V%u",
                 static_cast<unsigned>(BuildInfo::HardwareVariant));
        snprintf(firmwareVersion, sizeof(firmwareVersion), "FIRMWARE V%u.%u",
                 static_cast<unsigned>(BuildInfo::Major),
                 static_cast<unsigned>(BuildInfo::Minor));

        tft.fillScreen(COLOR_BLACK);
        tft.setFont(&universalis12);
        const auto drawCentered = [&tft](const char *text, int16_t y, uint16_t color) {
            int16_t x1 = 0;
            int16_t y1 = 0;
            uint16_t textWidth = 0;
            uint16_t textHeight = 0;
            tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
            tft.setTextColor(color);
            tft.setCursor((tft.width() - static_cast<int16_t>(textWidth)) / 2 - x1, y - y1);
            tft.print(text);
        };

        drawCentered(elevatorName, 39, COLOR_YELLOW);
        drawCentered("PANOU DUPLEX", 67, COLOR_WHITE);
        drawCentered(hardwareVersion, 104, COLOR_GREY);
        drawCentered(firmwareVersion, 131, COLOR_GREEN);
    }

    void init()
    {
        pinMode(Pins::UI::BACKLIGHT, OUTPUT);
        pinMode(Pins::UI::RCWL, INPUT);
        pinMode(Pins::UI::BUTTON, INPUT_PULLUP);
        pinMode(Pins::UI::BUTTON_SVC, INPUT_PULLUP);
        pinMode(Pins::UI::BUTTON_DEV, INPUT_PULLUP);
        digitalWrite(Pins::UI::BACKLIGHT, HIGH);
        backlightOn = true;
        lastActivityMillis = millis();

        // TFT1 -> SPI0
        SPI.setTX(Pins::TFT1::MOSI);
        SPI.setSCK(Pins::TFT1::SCK);

        // TFT2 -> SPI1
        SPI1.setTX(Pins::TFT2::MOSI);
        SPI1.setSCK(Pins::TFT2::SCK);

        // SPI speeds
        tft1.setSPISpeed(Config::Display::SPI_CLOCK);
        tft2.setSPISpeed(Config::Display::SPI_CLOCK);

        // CS idle state
        pinMode(Pins::TFT1::CS, OUTPUT);
        pinMode(Pins::TFT2::CS, OUTPUT);
        digitalWrite(Pins::TFT1::CS, HIGH);
        digitalWrite(Pins::TFT2::CS, HIGH);

        initializeControllers();

        const bool cabOrderSwapped = Settings::cabOrderSwapped();
        drawBootScreen(tft1, cabOrderSwapped ? "ASCENSOR 2" : "ASCENSOR 1");
        drawBootScreen(tft2, cabOrderSwapped ? "ASCENSOR 1" : "ASCENSOR 2");
        delay(5000);
    }

    // --- Service Mode control ---
    void setServiceMode(bool active) {
        serviceModeActive = active;
        if (active) {
            setBacklight(true);
            standbyActive = false;
        } else {
            lastActivityMillis = millis();
        }
    }

    bool isServiceMode() {
        return serviceModeActive;
    }

    bool update(const SharedPanel &localPanel) {
        (void)localPanel;

        const bool motionDetected = digitalRead(Pins::UI::RCWL) == HIGH;
        const bool buttonPressed  = digitalRead(Pins::UI::BUTTON) == LOW;

        if (motionDetected || buttonPressed) {
            lastActivityMillis = millis();

            if (standbyActive) {
                setBacklight(false);
                standbyActive = false;
                return true;
            }
            setBacklight(true);
            return false;
        }

        if (!serviceModeActive && (millis() - lastActivityMillis >= Settings::standbyTimeoutMs())) {
            if (!standbyActive) {
                setBacklight(false);
                tft1.fillScreen(COLOR_BLACK);
                tft2.fillScreen(COLOR_BLACK);
                initializeControllers();
                standbyActive = true;
            }
        }

        return false;
    }

    void showBacklight() {
        setBacklight(true);
    }

    // --- IMPLEMENTATION OF THE TECHNICAL DASHBOARD PRIMITIVES ---

    void clearTargetScreen(DisplayTarget target) {
        if (target == DisplayTarget::Left) { tft1.fillScreen(COLOR_BLACK); }
        else                               { tft2.fillScreen(COLOR_BLACK); }
    }

    void reinitializeTarget(DisplayTarget target) {
        Custom_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        initializeController(tft);
        tft.fillScreen(COLOR_BLACK);
    }

    void drawColorTestPage(DisplayTarget target, uint8_t colorIndex, bool labelAtTop) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        static const uint16_t colors[] = {
            COLOR_RED, COLOR_GREEN, 0x001F, COLOR_WHITE, COLOR_BLACK
        };
        static const char* labels[] = {"R", "G", "B", "W", "Black"};
        const uint8_t index = colorIndex < 5 ? colorIndex : 0;
        const uint16_t background = colors[index];
        const uint16_t foreground = (index == 1 || index == 3) ? COLOR_BLACK : COLOR_WHITE;

        tft.fillScreen(background);
        tft.setFont(&universalis12);
        tft.setTextSize(1);
        tft.setTextWrap(false);

        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t textWidth = 0;
        uint16_t textHeight = 0;
        tft.getTextBounds(labels[index], 0, 0, &x1, &y1, &textWidth, &textHeight);
        const int16_t textTop = labelAtTop ? 8 : static_cast<int16_t>(tft.height()) - 8 - textHeight;
        tft.setTextColor(foreground);
        tft.setCursor((tft.width() - static_cast<int16_t>(textWidth)) / 2 - x1, textTop - y1);
        tft.print(labels[index]);
    }

    // ← MODIFICAT: separator configurabil (culoare + grosime)
    void printMenuHeader(DisplayTarget target, const char* titluPagina,
                         uint16_t sepColor, uint8_t sepThickness) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        tft.setTextColor(COLOR_YELLOW);
        int16_t textX = 0;
        int16_t textY = 0;
        uint16_t textWidth = 0;
        uint16_t textHeight = 0;
        tft.getTextBounds(titluPagina, 0, 0, &textX, &textY, &textWidth, &textHeight);
        const int16_t centeredX = (128 - static_cast<int16_t>(textWidth)) / 2 - textX;
        const int16_t centeredY = (24 - static_cast<int16_t>(textHeight)) / 2 - textY;
        tft.setCursor(centeredX, centeredY);
        tft.print(titluPagina);

        // Separator ancorat la y=24, deseneaza in sus
        // sepThickness=3 → y=24,23,22 (compatibil cu versiunea anterioara)
        // sepThickness=2 → y=24,23 (+1px spatiu liber la y=22)
        for (uint8_t i = 0; i < sepThickness; i++) {
            tft.drawFastHLine(0, 24 - i, 128, sepColor);
        }
    }

    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 24 + (linie * 15);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(4, yPos);
        tft.print(eticheta);

        if (strcmp(eticheta, "Timp Function.:") == 0 || strcmp(eticheta, "Uptime       :") == 0) {
            uint16_t ore = valoare / 3600;
            uint8_t min = (valoare % 3600) / 60;
            char tBuf[16];
            snprintf(tBuf, sizeof(tBuf), "%02dh %02dm", ore, min);
            tft.setTextColor(COLOR_GREEN);
            tft.setCursor(74, yPos);
            tft.print(tBuf);
        } else {
            tft.setTextColor(culoareValoare);
            tft.setCursor(84, yPos);
            tft.print(valoare);
        }
    }

    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 24 + (linie * 15);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(4, yPos);
        tft.print(eticheta);

        tft.setTextColor(culoareValoare);
        tft.setCursor(84, yPos);
        tft.print(valoareText);
    }

    void printCommLabel(DisplayTarget target, uint8_t linie, const char* eticheta, uint16_t culoareEticheta) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&FreeSans9pt7b);
        const uint8_t yPos = 22 + (linie * 25);

        tft.setTextColor(culoareEticheta);
        tft.setCursor(4, yPos);
        tft.print(eticheta);
    }

    void printCommValue(DisplayTarget target, uint8_t linie, uint32_t valoareVeche,
                        uint32_t valoareNoua, uint16_t culoareValoare) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&FreeSans9pt7b);
        const uint8_t yPos = 22 + (linie * 25);

        char oldText[11];
        char valueText[11];
        snprintf(oldText, sizeof(oldText), "%lu", static_cast<unsigned long>(valoareVeche));
        snprintf(valueText, sizeof(valueText), "%lu", static_cast<unsigned long>(valoareNoua));

        int16_t oldX1 = 0;
        int16_t oldY1 = 0;
        uint16_t oldWidth = 0;
        uint16_t oldHeight = 0;
        tft.getTextBounds(oldText, 0, 0, &oldX1, &oldY1, &oldWidth, &oldHeight);

        int16_t newX1 = 0;
        int16_t newY1 = 0;
        uint16_t newWidth = 0;
        uint16_t newHeight = 0;
        tft.getTextBounds(valueText, 0, 0, &newX1, &newY1, &newWidth, &newHeight);

        tft.setTextColor(COLOR_BLACK);
        tft.setCursor(123 - oldX1 - static_cast<int16_t>(oldWidth), yPos);
        tft.print(oldText);

        tft.setTextColor(culoareValoare);
        tft.setCursor(123 - newX1 - static_cast<int16_t>(newWidth), yPos);
        tft.print(valueText);
    }

    void printMenuLineHex(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoareHex) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 24 + (linie * 15);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(4, yPos);
        tft.print(eticheta);

        tft.setTextColor(COLOR_GREY);
        tft.setCursor(64, yPos);
        tft.print("0x");
        tft.print(valoareHex, HEX);
    }

    void printMenuFooterDecoration(DisplayTarget target, const char* numarPaginaText) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 154;

        tft.drawFastHLine(0, 142, 128, COLOR_GREY);

        tft.setTextColor(COLOR_GREY);
        tft.setCursor(4, yPos);
        tft.print("b Prev");

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(54, yPos);
        tft.print(numarPaginaText);

        tft.setTextColor(COLOR_GREY);
        tft.setCursor(94, yPos);
        tft.print("Next a");
    }

    void drawText(DisplayTarget target, int16_t x, int16_t y,
        const GFXfont* font, const char* text, uint16_t color) {
        if (!font || !text) return;
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(font);
        tft.setTextColor(color);
        tft.setCursor(x, y);
        tft.print(text);
    }

    void drawHLine(DisplayTarget target, int16_t x, int16_t y, int16_t w, uint16_t color) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.drawFastHLine(x, y, w, color);
    }

    // --- Service overlay: chenar 3px, FĂRĂ text ---
    void drawServiceOverlay(DisplayTarget target, uint16_t color) {
        Custom_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.drawRect(0, 0, 128, 160, color);
        tft.drawRect(1, 1, 126, 158, color);
        tft.drawRect(2, 2, 124, 156, color);
    }

} // namespace Display