#ifndef BATTERY_CHECK_SCREEN_H
#define BATTERY_CHECK_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "battery/IBatteryProvider.h"

// ============================================================================
// Pagină UI "Battery Check".
//
// Afișează nivelul bateriei și starea de încărcare. Pagina este exclusiv
// informativă: nu are butoane, controale sau gesturi touchscreen.
//
// Comportament:
//   - Rămâne activă 5 secunde după activare/reactivare.
//   - După timeout fără atingere, partea grafică principală se șterge și
//     rămâne doar un punct indicator colorat în funcție de nivel.
//   - O atingere reaprinde pagina și resetează timerul.
//   - Dacă nivelul este MEDIUM/GOOD și timeout-ul a expirat, semnalează
//     către main.cpp că trebuie avansat către pagina următoare (START).
//   - Dacă nivelul este LOW sau bateria este în încărcare, rămâne în
//     Battery Check și NU avansează automat.
//
// Depinde doar de IBatteryProvider, deci poate fi testată cu stub.
// ============================================================================

enum class BatteryLevelState {
    LOW,
    MEDIUM,
    GOOD
};

class BatteryCheckScreen {
public:
    BatteryCheckScreen(Adafruit_GFX* tft, IBatteryProvider& battery);

    void init();
    void render(bool forceRedraw = false);

    // Procesează inputul tactil și temporizarea.
    // Returnează true când pagina a fost reactivată (pentru diagnostic).
    bool update(int touchX, int touchY, bool isTouched);

    // Returnează true dacă condițiile pentru avansare automată către START
    // sunt îndeplinite (MEDIUM/GOOD după timeout).
    bool shouldAdvance() const;

private:
    Adafruit_GFX* _tft;
    IBatteryProvider& _battery;

    bool _isRendered;
    bool _screenOn;
    bool _advancePending;

    unsigned long _lastActivityMs;
    unsigned long _lastBatteryUpdateMs;

    uint8_t  _displayedLevel;
    bool     _displayedCharging;

    // Constante vizuale și de timing
    static constexpr uint8_t  LOW_THRESHOLD_PERCENT    = 20;
    static constexpr uint8_t  MEDIUM_THRESHOLD_PERCENT = 60;
    static constexpr unsigned long SCREEN_ON_TIMEOUT_MS = 5000UL;
    static constexpr unsigned long BATTERY_UPDATE_MS    = 1500UL;

    static constexpr uint16_t COLOR_BACKGROUND = 0x0000; // Black
    static constexpr uint16_t COLOR_TEXT_MAIN  = 0xFFFF; // White
    static constexpr uint16_t COLOR_TEXT_MUTED = 0x9DF3; // Gray
    static constexpr uint16_t COLOR_RED        = 0xF800; // Red
    static constexpr uint16_t COLOR_YELLOW     = 0xFFE0; // Yellow
    static constexpr uint16_t COLOR_GREEN      = 0x07E0; // Green

    static constexpr int INDICATOR_RADIUS = 8;

    void drawMainContent();
    void drawIndicator();
    void clearScreenKeepIndicator();

    BatteryLevelState evaluateLevel(uint8_t percent) const;
    uint16_t levelStateToColor(BatteryLevelState state) const;
};

#endif // BATTERY_CHECK_SCREEN_H
