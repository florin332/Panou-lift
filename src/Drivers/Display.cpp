// Drivers/Display.cpp (Motorul Grafic Complet

#include "Display.h"

// 1. Core definitions loaded via relative subfolder paths
#include "Pins.h"
#include "Config.h"
#include "SharedPanel.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
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
    // ← SCHIMBAT: Adafruit_ST7735 → Custom_ST7735
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

    static void setBacklight(bool enabled) {
        if (backlightOn == enabled) return;
        digitalWrite(Pins::UI::BACKLIGHT, enabled ? HIGH : LOW);
        backlightOn = enabled;
    }

    static void initializeControllers() {
        tft1.initR(INITR_BLACKTAB);
        tft2.initR(INITR_BLACKTAB);

        tft1.setRowStart(Config::Display::ROW_START);
        tft1.setColStart(Config::Display::COL_START);
        tft2.setRowStart(Config::Display::ROW_START);
        tft2.setColStart(Config::Display::COL_START);

        tft1.setRotation(Config::Display::ROTATION);
        tft2.setRotation(Config::Display::ROTATION);

        tft1.setSPISpeed(Config::Display::SPI_CLOCK);
        tft2.setSPISpeed(Config::Display::SPI_CLOCK);
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

    // =========================================================
    // TFT1 -> SPI0
    // =========================================================
    SPI.setTX(Pins::TFT1::MOSI);
    SPI.setSCK(Pins::TFT1::SCK);

    // =========================================================
    // TFT2 -> SPI1
    // =========================================================
    SPI1.setTX(Pins::TFT2::MOSI);
    SPI1.setSCK(Pins::TFT2::SCK);

    // =========================================================
    // SPI speeds - identical to known-good V9.01
    // =========================================================
    tft1.setSPISpeed(Config::Display::SPI_CLOCK);
    tft2.setSPISpeed(Config::Display::SPI_CLOCK);

    // =========================================================
    // CS idle state
    // =========================================================
    pinMode(Pins::TFT1::CS, OUTPUT);
    pinMode(Pins::TFT2::CS, OUTPUT);

    digitalWrite(Pins::TFT1::CS, HIGH);
    digitalWrite(Pins::TFT2::CS, HIGH);

    initializeControllers();

    // =========================================================
    // Initial clear
    // =========================================================
    tft1.fillScreen(COLOR_BLACK);
    tft2.fillScreen(COLOR_BLACK);
}

    bool update(const SharedPanel &localPanel) {
        (void)localPanel;

        const bool motionDetected = digitalRead(Pins::UI::RCWL) == HIGH;
        const bool buttonPressed = digitalRead(Pins::UI::BUTTON) == LOW;

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

        if (millis() - lastActivityMillis >= Config::Timing::SCREEN_TIMEOUT_MS) {
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

        // --- IMPLEMENTATION OF THE TECHNICAL DASHBOARD PRIMITIVES V10.26 ---

    void clearTargetScreen(DisplayTarget target) {
        if (target == DisplayTarget::Left) { tft1.fillScreen(COLOR_BLACK); } 
        else                               { tft2.fillScreen(COLOR_BLACK); }
    }

    void printMenuHeader(DisplayTarget target, const char* titluPagina) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        tft.setTextColor(COLOR_YELLOW); 
        tft.setCursor(4, 18);
        tft.print(titluPagina);
        tft.drawFastHLine(0, 24, 128, COLOR_GREY); 
    }

    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 24 + (linie * 15);
        
        tft.setTextColor(COLOR_WHITE); // Label is always white
        tft.setCursor(4, yPos);
        tft.print(eticheta);
        
        if (strcmp(eticheta, "Timp Function.:") == 0 || strcmp(eticheta, "Uptime       :") == 0) {
            uint16_t ore = valoare / 3600;
            uint8_t min = (valoare % 3600) / 60;
            char tBuf[16];
            snprintf(tBuf, sizeof(tBuf), "%02dh %02dm", ore, min);
            tft.setTextColor(COLOR_GREEN); // Stable uptime is green
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

    void printMenuLineHex(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoareHex) {
        Adafruit_ST7735 &tft = (target == DisplayTarget::Left) ? tft1 : tft2;
        tft.setFont(&universalis12);
        uint8_t yPos = 24 + (linie * 15);
        
        tft.setTextColor(COLOR_WHITE); 
        tft.setCursor(4, yPos);
        tft.print(eticheta);
        
        tft.setTextColor(COLOR_GREY); // Engineering memory values use stable grey
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
        
        tft.setTextColor(COLOR_WHITE); // Dynamic index is crisp white
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

    

} // Closes namespace Display


