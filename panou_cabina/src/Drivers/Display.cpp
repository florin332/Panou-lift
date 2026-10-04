// Drivers/Display.cpp (Motorul Grafic Complet - V10.29 + Service Overlay)

#include "Display.h"
#include "DisplayGeometry.h"

// 1. Core definitions loaded via relative subfolder paths
#include "Pins.h"
#include "Config.h"
#include "SharedPanel.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
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
    static Adafruit_ILI9341 tft1(
        &SPI,
        Pins::TFT1::DC,
        Pins::TFT1::CS,
        Pins::TFT1::RST);

    // Isolated internal color constants mapped for 16-bit 565 format execution
    constexpr uint16_t COLOR_BLACK  = 0x0000;
    constexpr uint16_t COLOR_WHITE  = 0xFFFF;
    constexpr uint16_t COLOR_GREY   = 0x7BEF;
    constexpr uint16_t COLOR_YELLOW = 0xFFE0;
    constexpr uint16_t COLOR_GREEN  = 0x07E0;
    constexpr uint16_t COLOR_ORANGE = 0xFD20;
    constexpr uint16_t COLOR_RED    = 0xF800;

    static bool backlightOn = true;
    static bool serviceModeActive = false;

    static void setBacklight(bool enabled) {
        if (backlightOn == enabled) return;
        digitalWrite(Pins::UI::BACKLIGHT, enabled ? HIGH : LOW);
        backlightOn = enabled;
    }

    static void initializeControllers() {
        tft1.begin(Config::Display::SPI_CLOCK);
        tft1.invertDisplay(true);
        tft1.setRotation(Config::Display::ROTATION);
        DisplayGeom::begin(&tft1);  // HAL de layout: rezoluție + rotație reale
    }

    void init()
    {
        pinMode(Pins::UI::BACKLIGHT, OUTPUT);
        digitalWrite(Pins::UI::BACKLIGHT, HIGH);
        backlightOn = true;

        SPI.setTX(Pins::TFT1::MOSI);
        SPI.setSCK(Pins::TFT1::SCK);
        pinMode(Pins::TFT1::CS, OUTPUT);
        digitalWrite(Pins::TFT1::CS, HIGH);

        initializeControllers();
        tft1.fillScreen(COLOR_BLACK);
    }

    // --- Service Mode control ---
    void setServiceMode(bool active) {
        serviceModeActive = active;
        if (active) {
            setBacklight(true);
        }
    }

    bool isServiceMode() {
        return serviceModeActive;
    }

    bool update(const SharedPanel &localPanel) {
        (void)localPanel;
        setBacklight(true);
        return false;
    }

    void showBacklight() {
        setBacklight(true);
    }

    // --- IMPLEMENTATION OF THE TECHNICAL DASHBOARD PRIMITIVES ---

    void clearTargetScreen(DisplayTarget target) {
        (void)target;
        tft1.fillScreen(COLOR_BLACK);
    }

    // ← MODIFICAT: separator configurabil (culoare + grosime)
    // Titlul e centrat pe orizontală din metricile reale fontului și ale ecranului;
    // separatorul stă la o înălțime proporțională (11% din ecran), nu hardcodată.
    void printMenuHeader(DisplayTarget target, const char* titluPagina,
                         uint16_t sepColor, uint8_t sepThickness) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        const int16_t w = DisplayGeom::screenW();
        const int16_t h = DisplayGeom::screenH();

        const int16_t headerH = (11 * h) / 100;              // banda header: 11% din înălțime
        const int16_t x = DisplayGeom::cursorXForCenter(&universalis12, titluPagina, w / 2);
        const int16_t y = DisplayGeom::baselineForVCenter(&universalis12, titluPagina, headerH / 2);
        tft.setFont(&universalis12);
        tft.setTextColor(COLOR_YELLOW);
        tft.setCursor(x, y);
        tft.print(titluPagina);

        // Separator la baza benzii de header, desenat în sus cu sepThickness px
        for (uint8_t i = 0; i < sepThickness; i++) {
            tft.drawFastHLine(0, headerH - 1 - i, w, sepColor);
        }
    }

    // Geometrie linii de meniu (procente din înălțime, pas constant între linii)
    static inline int16_t menuLineY(uint8_t linie) {
        // prima linie la ~13% din înălțime, pas de 10% (42/320, 74/320... istoric)
        return DisplayGeom::screenH() * (13 + linie * 10) / 100;
    }
    static inline int16_t commLineY(uint8_t linie) {
        // prima linie la ~22% din înălțime, pas de 13% (72/320, 114/320... istoric)
        return DisplayGeom::screenH() * (22 + linie * 13) / 100;
    }
    static inline int16_t labelX()  { return DisplayGeom::screenW() * 3 / 100; }   // ~8/240
    static inline int16_t valueX()  { return DisplayGeom::screenW() * 59 / 100; }  // ~142/240
    static inline int16_t commValX(){ return DisplayGeom::screenW() * 44 / 100; }  // ~106/240

    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        uint16_t yPos = menuLineY(linie);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(labelX(), yPos);
        tft.print(eticheta);

        if (strcmp(eticheta, "Timp Function.:") == 0 || strcmp(eticheta, "Uptime       :") == 0) {
            uint16_t ore = valoare / 3600;
            uint8_t min = (valoare % 3600) / 60;
            char tBuf[16];
            snprintf(tBuf, sizeof(tBuf), "%02dh %02dm", ore, min);
            tft.setTextColor(COLOR_GREEN);
            tft.setCursor(valueX(), yPos);
            tft.print(tBuf);
        } else {
            tft.setTextColor(culoareValoare);
            tft.setCursor(valueX(), yPos);
            tft.print(valoare);
        }
    }

    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        uint16_t yPos = menuLineY(linie);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(labelX(), yPos);
        tft.print(eticheta);

        tft.setTextColor(culoareValoare);
        tft.setCursor(valueX(), yPos);
        tft.print(valoareText);
    }

    void printCommLine(DisplayTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        const uint16_t yPos = commLineY(linie);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(labelX(), yPos);
        tft.print(eticheta);

        tft.setTextColor(culoareValoare);
        tft.setCursor(commValX(), yPos);
        tft.print(valoareText);
    }

    void printCommLine(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        const uint16_t yPos = commLineY(linie);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(labelX(), yPos);
        tft.print(eticheta);

        tft.setTextColor(culoareValoare);
        tft.setCursor(commValX(), yPos);
        tft.print(valoare);
    }

    void printMenuLineHex(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoareHex) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        uint16_t yPos = menuLineY(linie);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(labelX(), yPos);
        tft.print(eticheta);

        tft.setTextColor(COLOR_GREY);
        tft.setCursor(valueX(), yPos);
        tft.print("0x");
        tft.print(valoareHex, HEX);
    }

    void printMenuFooterDecoration(DisplayTarget target, const char* numarPaginaText) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(&universalis12);
        const int16_t w = DisplayGeom::screenW();
        const int16_t h = DisplayGeom::screenH();
        const int16_t yPos = h * 96 / 100;          // ~306/320
        const int16_t ySep = h * 89 / 100;          // ~286/320

        tft.drawFastHLine(0, ySep, w, COLOR_GREY);

        tft.setTextColor(COLOR_GREY);
        tft.setCursor(labelX(), yPos);
        tft.print("b Prev");

        // numărul de pagină centrat pe ecran
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(DisplayGeom::cursorXForCenter(&universalis12, numarPaginaText, w / 2), yPos);
        tft.print(numarPaginaText);

        // "Next a" aliniat la dreapta cu aceeași margine ca labelX
        int16_t x1, y1; uint16_t tw, th;
        tft.getTextBounds("Next a", 0, 0, &x1, &y1, &tw, &th);
        tft.setTextColor(COLOR_GREY);
        tft.setCursor(w - labelX() - tw - x1, yPos);
        tft.print("Next a");
    }

    void drawText(DisplayTarget target, int16_t x, int16_t y,
        const GFXfont* font, const char* text, uint16_t color) {
        if (!font || !text) return;
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.setFont(font);
        tft.setTextColor(color);
        tft.setCursor(x, y);
        tft.print(text);
    }

    void drawHLine(DisplayTarget target, int16_t x, int16_t y, int16_t w, uint16_t color) {
        (void)target;
        Adafruit_ILI9341 &tft = tft1;
        tft.drawFastHLine(x, y, w, color);
    }

    // --- Service overlay: chenar 3px, FĂRĂ text, dimensiuni din HAL ---
    void drawServiceOverlay(DisplayTarget target, uint16_t color) {
        (void)target;
        const int16_t w = DisplayGeom::screenW();
        const int16_t h = DisplayGeom::screenH();
        for (int16_t i = 0; i < 3; i++) {
            tft1.drawRect(i, i, w - 2 * i, h - 2 * i, color);
        }
    }

} // namespace Display