#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "SharedPanel.h"
#include <gfxfont.h>
#include <stdint.h>

enum class GraphicsTarget : uint8_t {
    Panel
};

namespace Graphics
{
    void init();
    bool update(const SharedPanel &localPanel);
    void showBacklight();
    void showOverloadImage();
    void hideOverloadImage();

    void setServiceMode(bool active);
    bool isServiceMode();

    void reinitialize();
    void clearTargetScreen(GraphicsTarget target);
    void drawColorTestPage(GraphicsTarget target, uint8_t colorIndex, bool labelAtTop);
    void printMenuHeader(GraphicsTarget target, const char* title,
                         uint16_t separatorColor = 0x7BEF, uint8_t separatorThickness = 3);
    void printMenuLine(GraphicsTarget target, uint8_t line, const char* label, uint32_t value);
    void printMenuLine(GraphicsTarget target, uint8_t line, const char* label, const char* valueText);
    void printMenuLineExt(GraphicsTarget target, uint8_t line, const char* label, uint32_t value, uint16_t valueColor);
    void printMenuLineExt(GraphicsTarget target, uint8_t line, const char* label, const char* valueText, uint16_t valueColor);
    void printCommLine(GraphicsTarget target, uint8_t line, const char* label, const char* valueText, uint16_t valueColor);
    void printCommLine(GraphicsTarget target, uint8_t line, const char* label, uint32_t value, uint16_t valueColor);
    void printMenuLineHex(GraphicsTarget target, uint8_t line, const char* label, uint32_t valueHex);
    void printMenuFooterDecoration(GraphicsTarget target, const char* pageNumberText);
    void drawText(GraphicsTarget target, int16_t x, int16_t y,
                  const GFXfont* font, const char* text, uint16_t color);
    void drawHLine(GraphicsTarget target, int16_t x, int16_t y, int16_t width, uint16_t color);
    void drawServiceOverlay(GraphicsTarget target, uint16_t color);
}

#endif