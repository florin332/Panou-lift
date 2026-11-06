#include "Graphics.h"
#include "Display.h"

namespace Graphics
{
    void init() { Display::init(); }
    bool update(const SharedPanel &localPanel) { return Display::update(localPanel); }
    void showBacklight() { Display::showBacklight(); }
    void showOverloadImage() { Display::showOverloadImage(); }
    void hideOverloadImage() { Display::hideOverloadImage(); }
    void setServiceMode(bool active) { Display::setServiceMode(active); }
    bool isServiceMode() { return Display::isServiceMode(); }
    void clearTargetScreen(GraphicsTarget target) { Display::clearTargetScreen(target); }
    void printMenuHeader(GraphicsTarget target, const char* title, uint16_t separatorColor, uint8_t separatorThickness) {
        Display::printMenuHeader(target, title, separatorColor, separatorThickness);
    }
    void printMenuLine(GraphicsTarget target, uint8_t line, const char* label, uint32_t value) {
        Display::printMenuLine(target, line, label, value);
    }
    void printMenuLine(GraphicsTarget target, uint8_t line, const char* label, const char* valueText) {
        Display::printMenuLine(target, line, label, valueText);
    }
    void printMenuLineExt(GraphicsTarget target, uint8_t line, const char* label, uint32_t value, uint16_t valueColor) {
        Display::printMenuLineExt(target, line, label, value, valueColor);
    }
    void printMenuLineExt(GraphicsTarget target, uint8_t line, const char* label, const char* valueText, uint16_t valueColor) {
        Display::printMenuLineExt(target, line, label, valueText, valueColor);
    }
    void printCommLine(GraphicsTarget target, uint8_t line, const char* label, const char* valueText, uint16_t valueColor) {
        Display::printCommLine(target, line, label, valueText, valueColor);
    }
    void printCommLine(GraphicsTarget target, uint8_t line, const char* label, uint32_t value, uint16_t valueColor) {
        Display::printCommLine(target, line, label, value, valueColor);
    }
    void printMenuLineHex(GraphicsTarget target, uint8_t line, const char* label, uint32_t valueHex) {
        Display::printMenuLineHex(target, line, label, valueHex);
    }
    void printMenuFooterDecoration(GraphicsTarget target, const char* pageNumberText) {
        Display::printMenuFooterDecoration(target, pageNumberText);
    }
    void drawText(GraphicsTarget target, int16_t x, int16_t y,
                  const GFXfont* font, const char* text, uint16_t color) {
        Display::drawText(target, x, y, font, text, color);
    }
    void drawHLine(GraphicsTarget target, int16_t x, int16_t y, int16_t width, uint16_t color) {
        Display::drawHLine(target, x, y, width, color);
    }
    void drawServiceOverlay(GraphicsTarget target, uint16_t color) {
        Display::drawServiceOverlay(target, color);
    }
}