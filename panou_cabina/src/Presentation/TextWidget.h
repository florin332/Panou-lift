#ifndef TEXT_WIDGET_H
#define TEXT_WIDGET_H
#include <Adafruit_GFX.h>
#include <cstring>

// =============================================================================
// CONTRACT DE VIAȚĂ A TEXTULUI
// =============================================================================
// TextWidget păstrează un pointer const char*, NU copiază conținutul.
// Textul referit trebuie să rămână valid pe toată durata de viață a widgetului.
// Valabil pentru: floorStr[] (flash), string literals ("ASCENSOR 1", "DEFECT").
// NU folosiți buffere de stivă sau heap reutilizate fără persistență garantată.
// =============================================================================


struct TextWidget {
    const char* text = nullptr;
    const GFXfont* font = nullptr;
    int16_t x = -1;
    int16_t y = -1;
    uint16_t color = 0x0000;
    bool drawn = false;

    bool isDirty(const char* newText, const GFXfont* newFont,
                 int16_t newX, int16_t newY, uint16_t newColor) const {
        if (!drawn) return true;
        if (font != newFont) return true;
        if (x != newX || y != newY) return true;
        if (color != newColor) return true;
        if (text == nullptr || newText == nullptr) return true;
        return std::strcmp(text, newText) != 0;
    }

    void commit(const char* newText, const GFXfont* newFont,
                int16_t newX, int16_t newY, uint16_t newColor) {
        text = newText;
        font = newFont;
        x = newX;
        y = newY;
        color = newColor;
        drawn = true;
    }

    void reset() {
        text = nullptr;
        font = nullptr;
        x = -1;
        y = -1;
        color = 0x0000;
        drawn = false;
    }
};

#endif