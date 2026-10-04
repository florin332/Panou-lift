// Drivers/DisplayGeometry.h — HAL de layout pentru display
//
// Single source of truth pentru geometria ecranului și metricile textului.
// Toate pozițiile se calculează din:
//   1. rezoluția reală raportată de driver (tft.width() / tft.height())
//   2. metricile reale ale fontului (GFXglyph) pentru textul concret afișat
//
// Dacă se schimbă LCD-ul, rotația sau fonturile, pozițiile se adaptează
// automat — nu există coordonate hardcodate în straturile superioare.
//
// Convenția Adafruit GFX: setCursor(x, y) setează LINIA DE BAZĂ a textului.
// Glifa unui caracter 'c' ocupă, relativ la cursor:
//   x + glyph.xOffset  ..  x + glyph.xOffset + glyph.width   (orizontal)
//   y + glyph.yOffset  ..  y + glyph.yOffset + glyph.height  (vertical)

#ifndef DISPLAY_GEOMETRY_H
#define DISPLAY_GEOMETRY_H

#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <cstring>

namespace DisplayGeom
{
    // Bounding box al unui text, în pixeli de ecran, dacă ar fi desenat
    // cu baseline-ul la (x=0, y=0).
    struct TextBounds {
        int16_t left;    // cel mai din stânga pixel
        int16_t top;     // cel mai de sus pixel (negativ = deasupra baseline-ului)
        int16_t right;   // primul pixel DUPĂ text
        int16_t bottom;  // primul pixel SUB text
    };

    // Inițializare o singură dată, din Display::init(), cu driverul concret.
    void begin(Adafruit_GFX* display);

    // Dimensiunile reale ale ecranului (țin cont de rotație).
    int16_t screenW();
    int16_t screenH();
    inline int16_t centerX() { return screenW() / 2; }
    inline int16_t centerY() { return screenH() / 2; }

    // Măsoară textul cu fontul dat. Fallback pe font->yAdvance dacă glifa
    // lipsește din font (intervalul first..last). Text nullptr/gol → box gol.
    TextBounds measure(const GFXfont* font, const char* text);

    // Cursor x pentru ca textul să fie centrat orizontal pe axa centerAx.
    int16_t cursorXForCenter(const GFXfont* font, const char* text, int16_t centerAx);

    // Cursor y (baseline) pentru ca textul să fie centrat vertical pe centerAy.
    int16_t baselineForVCenter(const GFXfont* font, const char* text, int16_t centerAy);

    // Cursor y (baseline) pentru ca TOP-ul textului să fie la topY.
    int16_t baselineForTop(const GFXfont* font, const char* text, int16_t topY);

    // Cursor y (baseline) pentru ca BOTTOM-ul textului să fie la bottomY.
    int16_t baselineForBottom(const GFXfont* font, const char* text, int16_t bottomY);

    // true dacă textul desenat la (x, y) iese integral pe ecran.
    bool fitsOnScreen(const GFXfont* font, const char* text, int16_t x, int16_t y);
}

#endif // DISPLAY_GEOMETRY_H
