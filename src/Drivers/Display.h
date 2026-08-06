// Drivers/Display.h (Interfața Unificată - Observația 3)

#ifndef DISPLAY_H
#define DISPLAY_H

#include "SharedPanel.h"
#include <Adafruit_GFX.h>  // ← NOU: pentru GFXfont*

enum class DisplayTarget : uint8_t {
    Left,
    Right
};

namespace Display
{
    void init();
    void update(const SharedPanel &localPanel);

    void clearTargetScreen(DisplayTarget target);
    void printMenuHeader(DisplayTarget target, const char* titluPagina);
    void printMenuLine(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare);
    void printMenuLine(DisplayTarget target, uint8_t linie, const char* eticheta, const char* valoareText);
    
    // Extinderile cerute pentru randarea de Service și Developer
    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare);
    void printMenuLineExt(DisplayTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare);
    void printMenuLineHex(DisplayTarget target, uint8_t linie, const char* eticheta, uint32_t valoareHex);
    void printMenuFooterDecoration(DisplayTarget target, const char* numarPaginaText);

    // Primitivă de bază pentru text. Presentation decide când e erase (BLACK) și când e draw.
    void drawText(DisplayTarget target, int16_t x, int16_t y,
                  const GFXfont* font, const char* text, uint16_t color);
}

#endif // DISPLAY_H
