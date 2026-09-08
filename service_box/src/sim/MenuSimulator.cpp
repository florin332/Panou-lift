// ============================================================================
// PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU - PROVIZORIU
//
// MenuSimulator.cpp
//
// Implementare PROVIZORIE a simulatorului de display + touch.
// Vezi MenuSimulator.h pentru context. ELIMINAT la reintegrarea cu hardware.
// ============================================================================

#include "MenuSimulator.h"

// PROVIZORIU - instante globale
SimDisplay simDisplay;
SimTouch   simTouch;

// ============================================================================
// PROVIZORIU - SimDisplay
// ============================================================================

SimDisplay::SimDisplay()
    : Adafruit_GFX(SIM_SCREEN_WIDTH, SIM_SCREEN_HEIGHT),
      _textLen(0), _textCursorX(0), _textCursorY(0) {
    _textLine[0] = '\0';
}

void SimDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // PROVIZORIU - pixelii individuali NU sunt logati (ar inunda Serial)
    (void)x; (void)y; (void)color;
}

void SimDisplay::fillScreen(uint16_t color) {
    Serial.printf("[SIM LCD] fillScreen(color=0x%04X)\n", color); // PROVIZORIU
}

void SimDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    Serial.printf("[SIM LCD] fillRect(x=%d y=%d w=%d h=%d color=0x%04X)\n",
                  x, y, w, h, color); // PROVIZORIU
}

void SimDisplay::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    Serial.printf("[SIM LCD] fillRoundRect(x=%d y=%d w=%d h=%d r=%d color=0x%04X)\n",
                  x, y, w, h, r, color); // PROVIZORIU
}

void SimDisplay::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    Serial.printf("[SIM LCD] drawRoundRect(x=%d y=%d w=%d h=%d r=%d color=0x%04X)\n",
                  x, y, w, h, r, color); // PROVIZORIU
}

void SimDisplay::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    Serial.printf("[SIM LCD] fillCircle(x=%d y=%d r=%d color=0x%04X)\n",
                  x, y, r, color); // PROVIZORIU
}

size_t SimDisplay::write(uint8_t c) {
    // PROVIZORIU - reconstituie textul randat si il logheaza la newline
    if (_textLen == 0) {
        _textCursorX = getCursorX();
        _textCursorY = getCursorY();
    }
    if (c == '\n' || _textLen >= sizeof(_textLine) - 1) {
        _textLine[_textLen] = '\0';
        Serial.printf("[SIM LCD] text(x=%d y=%d): \"%s\"\n",
                      _textCursorX, _textCursorY, _textLine);
        _textLen = 0;
    } else if (c != '\r') {
        _textLine[_textLen++] = (char)c;
    }
    return 1;
}

// ============================================================================
// PROVIZORIU - SimTouch
// ============================================================================

SimTouch::SimTouch() : _touched(false), _x(0), _y(0), _cmdLen(0) {}

void SimTouch::update() {
    // PROVIZORIU - citire non-blocanta a comenzilor Serial
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            _cmdBuf[_cmdLen] = '\0';
            if (_cmdLen > 0) processCommand();
            _cmdLen = 0;
        } else if (_cmdLen < sizeof(_cmdBuf) - 1) {
            _cmdBuf[_cmdLen++] = c;
        }
    }
}

void SimTouch::processCommand() {
    // PROVIZORIU - formate acceptate: "t <x> <y>" si "up"
    if (strcmp(_cmdBuf, "up") == 0) {
        _touched = false;
        Serial.printf("[SIM TOUCH] release (last x=%d y=%d)\n", _x, _y);
        return;
    }
    int tx, ty;
    if (sscanf(_cmdBuf, "t %d %d", &tx, &ty) == 2) {
        _x = tx;
        _y = ty;
        _touched = true;
        Serial.printf("[SIM TOUCH] press x=%d y=%d\n", _x, _y);
        return;
    }
    Serial.printf("[SIM TOUCH] comanda necunoscuta: \"%s\" (foloseste: t <x> <y> / up)\n", _cmdBuf);
}

bool SimTouch::isTouched() const { return _touched; }
int  SimTouch::getX() const      { return _x; }
int  SimTouch::getY() const      { return _y; }
