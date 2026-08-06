// Drivers/Display.cpp (Motorul Grafic Complet

#include "Display.h"

// 1. Core definitions loaded via relative subfolder paths
#include "Pins.h"
#include "Config.h"
#include "SharedPanel.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
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
static Adafruit_ST7735 tft1 =
    Adafruit_ST7735(Pins::TFT1::CS,
                    Pins::TFT1::DC,
                    Pins::TFT1::RST);

static Adafruit_ST7735 tft2 =
    Adafruit_ST7735(&SPI1,
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

void init()
{
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
    tft1.setSPISpeed(8000000UL);
    tft2.setSPISpeed(8000000UL);

    // =========================================================
    // CS idle state
    // =========================================================
    pinMode(Pins::TFT1::CS, OUTPUT);
    pinMode(Pins::TFT2::CS, OUTPUT);

    digitalWrite(Pins::TFT1::CS, HIGH);
    digitalWrite(Pins::TFT2::CS, HIGH);

    // =========================================================
    // ST7735 initialization
    // =========================================================
    tft1.initR(INITR_BLACKTAB);
    tft2.initR(INITR_BLACKTAB);

    // =========================================================
    // Orientation
    // =========================================================
    tft1.setRotation(0);
    tft2.setRotation(0);

    // =========================================================
    // Initial clear
    // =========================================================
    tft1.fillScreen(COLOR_BLACK);
    tft2.fillScreen(COLOR_BLACK);
}

    void update(const SharedPanel &localPanel) {
        // High-frequency passenger screen updates execute natively right here
        // (This routine isolates standard floor arrows to eliminate active menu flickering)
        (void)localPanel;
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


