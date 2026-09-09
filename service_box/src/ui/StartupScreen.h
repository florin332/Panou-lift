#ifndef STARTUP_SCREEN_H
#define STARTUP_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "battery/IBatteryProvider.h"

// Decuplat de HAL pe branch-ul graphic_ui: primeste doar o instanta
// Adafruit_GFX (display real ST7789 pe Waveshare / simulator pe Marble)
// si o referinta la furnizorul de stare baterie.
//
// Politică baterie:
//   - START confirmă navigarea către Dashboard doar dacă starea bateriei
//     este NORMAL sau CHARGING.
//   - LOW sau CRITICAL blochează inițierea unui test nou (returnează false),
//     conform politicii de navigație. Testele în desfășurare NU sunt oprite
//     de această pagină; oprirea controlată la CRITICAL se face în main.cpp.

class StartupScreen {
private:
    Adafruit_GFX* _tft;
    IBatteryProvider& _battery;
    bool _isRendered;
    bool _buttonPressed;

    // Battery display tracking
    uint8_t _displayedLevel;
    bool _displayedCharging;
    unsigned long _lastBatteryUpdateMs;

    // Button display tracking (avoid redraw flicker).
    // Butonul nu are feedback vizual la apăsare, deci doar textul contează.
    String _displayedButtonText;

    // UI Geometry Constants (Portrait 240x320)
    const int BTN_X = 50;
    const int BTN_Y = 190; // Pozitie originala
    const int BTN_W = 140;
    const int BTN_H = 45;
    const int BTN_RADIUS = 6;

    // Battery indicator constants (same style as BatteryCheckScreen)
    static constexpr uint8_t LOW_THRESHOLD_PERCENT    = 20;
    static constexpr uint8_t MEDIUM_THRESHOLD_PERCENT = 60;
    static constexpr uint16_t COLOR_BACKGROUND = 0x0000; // Black
    static constexpr uint16_t COLOR_TEXT_MAIN  = 0xFFFF; // White
    static constexpr uint16_t COLOR_TEXT_MUTED = 0x9DF3; // Gray
    static constexpr uint16_t COLOR_BTN_BASE   = 0x1967; // Deep Blue
    static constexpr uint16_t COLOR_GREEN      = 0x07E0; // Bright Green
    static constexpr uint16_t COLOR_YELLOW     = 0xFFE0; // Yellow
    static constexpr uint16_t COLOR_RED        = 0xF800; // Red
    static constexpr int BATTERY_DOT_RADIUS = 4;
    static constexpr unsigned long BATTERY_UPDATE_MS = 1500UL;

    void drawButton(bool pressed);
    void drawBatteryInfo();
    uint16_t levelToColor(uint8_t percent) const;

public:
    StartupScreen(Adafruit_GFX* tftInstance, IBatteryProvider& battery);
    void init();
    void render(bool forceRedraw = false);
    bool update(int touchX, int touchY, bool isTouched); // Returns true when START is pressed & released
    void updateStorageInfo(uint32_t freeMB, uint32_t totalMB, bool cardAvailable);
};

#endif // STARTUP_SCREEN_H
