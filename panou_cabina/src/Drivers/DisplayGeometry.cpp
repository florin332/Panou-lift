// Drivers/DisplayGeometry.cpp — implementare HAL de layout

#include "DisplayGeometry.h"

namespace DisplayGeom
{
    static Adafruit_GFX* s_disp = nullptr;

    void begin(Adafruit_GFX* display) {
        s_disp = display;
    }

    int16_t screenW() { return s_disp ? s_disp->width()  : 240; }
    int16_t screenH() { return s_disp ? s_disp->height() : 320; }

    TextBounds measure(const GFXfont* font, const char* text) {
        TextBounds b{0, 0, 0, 0};
        if (!font || !text || !text[0]) return b;

        int16_t penX = 0;          // poziția cursorului (baseline y=0)
        bool first = true;

        for (const char* p = text; *p; ++p) {
            uint8_t c = static_cast<uint8_t>(*p);
            if (c < font->first || c > font->last) {
                // glifa lipsește: avansăm cu spațiul (primul caracter din font)
                penX += font->glyph[0].xAdvance;
                continue;
            }
            const GFXglyph& g = font->glyph[c - font->first];

            const int16_t left   = penX + g.xOffset;
            const int16_t right  = left + g.width;
            const int16_t top    = g.yOffset;             // relativ la baseline 0
            const int16_t bottom = g.yOffset + g.height;

            if (first) {
                b.left = left; b.right = right; b.top = top; b.bottom = bottom;
                first = false;
            } else {
                if (left   < b.left)   b.left   = left;
                if (right  > b.right)  b.right  = right;
                if (top    < b.top)    b.top    = top;
                if (bottom > b.bottom) b.bottom = bottom;
            }
            penX += g.xAdvance;
        }
        return b;
    }

    int16_t cursorXForCenter(const GFXfont* font, const char* text, int16_t centerAx) {
        TextBounds b = measure(font, text);
        return centerAx - (b.left + b.right) / 2;
    }

    int16_t baselineForVCenter(const GFXfont* font, const char* text, int16_t centerAy) {
        TextBounds b = measure(font, text);
        return centerAy - (b.top + b.bottom) / 2;
    }

    int16_t baselineForTop(const GFXfont* font, const char* text, int16_t topY) {
        TextBounds b = measure(font, text);
        return topY - b.top;
    }

    int16_t baselineForBottom(const GFXfont* font, const char* text, int16_t bottomY) {
        TextBounds b = measure(font, text);
        return bottomY - b.bottom;
    }

    bool fitsOnScreen(const GFXfont* font, const char* text, int16_t x, int16_t y) {
        TextBounds b = measure(font, text);
        return (x + b.left >= 0) && (x + b.right <= screenW())
            && (y + b.top >= 0) && (y + b.bottom <= screenH());
    }
}
