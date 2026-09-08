// ============================================================================
// PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU
//
// MenuSimulator.h
//
// Simulator de display + touch pentru dezvoltarea meniului FARA hardware.
// Inlocuieste temporar HAL-ul eliminat de pe acest branch (graphic_ui).
//
// Pe acest branch:
//   - SERVICEBOX_WAVESHARE foloseste hardware REAL (BSP + ST7789 + CST328)
//   - SERVICEBOX_MARBLE foloseste acest SIMULATOR pana la integrarea
//     driverelor Marble de pe branch-ul hardware dedicat
//
// TOT continutul acestui fisier este PROVIZORIU si trebuie ELIMINAT
// cand meniul este reintegrat pe branch-urile hardware.
//
// Display: subclasa Adafruit_GFX (240x320) care NU deseneaza nimic fizic;
//          operatiile de desenare sunt logate pe Serial in forma text.
// Touch:   simulat prin comenzi primite pe Serial (vezi MenuSimulator.cpp).
//
// Cauta "PROVIZORIU" in tot proiectul pentru a gasi usor tot ce trebuie
// eliminat la reintegrarea cu hardware-ul real.
// ============================================================================

#ifndef MENU_SIMULATOR_H
#define MENU_SIMULATOR_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

// PROVIZORIU - dimensiunile ecranului tinta (portrait), folosite de simulator
#define SIM_SCREEN_WIDTH  240
#define SIM_SCREEN_HEIGHT 320

// ============================================================================
// PROVIZORIU - Display simulat (nu exista ecran fizic in acest mod)
// ============================================================================
class SimDisplay : public Adafruit_GFX {
public:
    SimDisplay();

    // Obligatoriu pentru Adafruit_GFX; PROVIZORIU - nu deseneaza nimic
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    // Virtuale in Adafruit_GFX - logheaza operatia in loc sa o execute,
    // pentru a nu inunda Serial cu cate un mesaj per pixel
    void fillScreen(uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    size_t write(uint8_t c) override;

    // PROVIZORIU - supraincarcari ale helper-elor NE-VIRTUALE din Adafruit_GFX
    // (fara 'override': in clasa de baza sunt metode obisnuite care apeleaza
    // drawPixel/fillRect; le supraincarcam doar pentru logging)
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);

private:
    // PROVIZORIU - buffer linie pentru reconstituirea textului "afisat"
    char    _textLine[64];
    uint8_t _textLen;
    int16_t _textCursorX;
    int16_t _textCursorY;
};

// ============================================================================
// PROVIZORIU - Touch simulat prin Serial
// ============================================================================
// Comenzi (o linie pe Serial, terminata cu Enter):
//   "t <x> <y>"  = deget apasat la coordonatele (x, y)
//   "up"         = deget ridicat
// Exemplu apasare buton START:  t 120 210   apoi   up
// ============================================================================
class SimTouch {
public:
    SimTouch();

    // PROVIZORIU - citeste comenzile Serial disponibile (non-blocant)
    void update();

    bool isTouched() const;
    int  getX() const;
    int  getY() const;

private:
    // PROVIZORIU
    bool _touched;
    int  _x;
    int  _y;
    char _cmdBuf[32];
    uint8_t _cmdLen;

    void processCommand(); // PROVIZORIU
};

// PROVIZORIU - instante globale ale simulatorului
extern SimDisplay simDisplay;
extern SimTouch   simTouch;

#endif // MENU_SIMULATOR_H
