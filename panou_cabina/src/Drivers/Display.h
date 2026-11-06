// Drivers/Display.h (Interfața Unificată - V10.29 + Service Overlay)

#ifndef DISPLAY_H
#define DISPLAY_H

#include "Graphics.h"

namespace Display
{
    void init();
    bool update(const SharedPanel &localPanel);
    void showBacklight();
    void showOverloadImage();
    void hideOverloadImage();

    // Service Mode control
    void setServiceMode(bool active);
    bool isServiceMode();

    void clearTargetScreen(GraphicsTarget target);

    // Separator configurabil: culoare (default gri) si grosime in px (default 3)
    void printMenuHeader(GraphicsTarget target, const char* titluPagina,
                         uint16_t sepColor = 0x7BEF, uint8_t sepThickness = 3);

    void printMenuLine(GraphicsTarget target, uint8_t linie, const char* eticheta, uint32_t valoare);
    void printMenuLine(GraphicsTarget target, uint8_t linie, const char* eticheta, const char* valoareText);

    // Extinderile cerute pentru randarea de Service și Developer
    void printMenuLineExt(GraphicsTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare);
    void printMenuLineExt(GraphicsTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare);
    void printCommLine(GraphicsTarget target, uint8_t linie, const char* eticheta, const char* valoareText, uint16_t culoareValoare);
    void printCommLine(GraphicsTarget target, uint8_t linie, const char* eticheta, uint32_t valoare, uint16_t culoareValoare);
    void printMenuLineHex(GraphicsTarget target, uint8_t linie, const char* eticheta, uint32_t valoareHex);
    void printMenuFooterDecoration(GraphicsTarget target, const char* numarPaginaText);

    // Primitivă de bază pentru text. Presentation decide când e erase (BLACK) și când e draw.
    void drawText(GraphicsTarget target, int16_t x, int16_t y,
        const GFXfont* font, const char* text, uint16_t color);

    // Primitivă de linie orizontală
    void drawHLine(GraphicsTarget target, int16_t x, int16_t y, int16_t w, uint16_t color);

    // Service overlay: chenar subțire (2px) fără text
    void drawServiceOverlay(GraphicsTarget target, uint16_t color);
}

#endif // DISPLAY_H