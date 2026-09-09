#include "BatteryCheckScreen.h"

BatteryCheckScreen::BatteryCheckScreen(Adafruit_GFX* tft, IBatteryProvider& battery)
    : _tft(tft)
    , _battery(battery)
    , _isRendered(false)
    , _screenOn(true)
    , _advancePending(false)
    , _lastActivityMs(0)
    , _lastBatteryUpdateMs(0)
    , _standbyBlinkTimerMs(0)
    , _standbyBlinkOn(false)
    , _standbyIndicatorVisible(false)
    , _displayedLevel(0xFF)
    , _displayedCharging(false)
{
}

void BatteryCheckScreen::init()
{
    _isRendered = false;
    _screenOn = true;
    _advancePending = false;
    _lastActivityMs = millis();
    _lastBatteryUpdateMs = 0;
    _standbyBlinkTimerMs = 0;
    _standbyBlinkOn = false;
    _standbyIndicatorVisible = false;
    _displayedLevel = 0xFF;
    _displayedCharging = false;
}

void BatteryCheckScreen::render(bool forceRedraw)
{
    if (_screenOn) {
        if (_isRendered && !forceRedraw) {
            return;
        }
        drawMainContent();
        _isRendered = true;
    }
    else {
        // Stare standby: clipitul este gestionat de update() (timing).
        // Aici redesenăm doar dacă nivelul sau starea de încărcare s-a
        // schimbat între două apeluri update() (caz defensiv).
        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();

        bool batteryChanged = (currentLevel != _displayedLevel) ||
                              (currentCharging != _displayedCharging);

        if (batteryChanged) {
            _displayedLevel = currentLevel;
            _displayedCharging = currentCharging;
            drawStandbyContent();
        }
    }
}

bool BatteryCheckScreen::update(int touchX, int touchY, bool isTouched)
{
    unsigned long now = millis();
    bool wasReactivated = false;

    if (isTouched) {
        if (!_screenOn) {
            _screenOn = true;
            _isRendered = false;
            wasReactivated = true;
        }
        _lastActivityMs = now;
        _advancePending = false;
    }

    // Actualizare periodică a valorii bateriei (1..2 secunde).
    if (now - _lastBatteryUpdateMs >= BATTERY_UPDATE_MS) {
        _lastBatteryUpdateMs = now;

        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();

        if (currentLevel != _displayedLevel || currentCharging != _displayedCharging) {
            _isRendered = false;  // Forțează re-desenare cu noile valori.

            _displayedLevel = currentLevel;
            _displayedCharging = currentCharging;

            // Dacă ecranul este în standby, re-desenăm conținutul standby
            // pentru a reflecta noul nivel imediat.
            if (!_screenOn) {
                _standbyBlinkTimerMs = now;
                _standbyBlinkOn = true;
                _standbyIndicatorVisible = true; // punctul pornește aprins
                drawStandbyContent();
            }
        }
    }

    // Clipitul standby avansează aici (logică de timing), nu în render().
    // Astfel nu depindem de faptul că main.cpp apelează render() în fiecare
    // iterație de loop cât ecranul este în standby.
    if (!_screenOn && updateStandbyBlinkPhase()) {
        drawStandbyContent();
    }

    // Timeout de 5 secunde fără atingere.
    if (_screenOn && (now - _lastActivityMs >= SCREEN_ON_TIMEOUT_MS)) {
        BatteryState state = _battery.getState();

        if (state == BatteryState::NORMAL) {
            // Nivel suficient: avansare automată către START.
            _advancePending = true;
        }
        else {
            // LOW, CRITICAL sau CHARGING: rămâne în Battery Check.
            _screenOn = false;
            _isRendered = false;
            clearScreenKeepIndicator();
        }
    }

    return wasReactivated;
}

bool BatteryCheckScreen::shouldAdvance() const
{
    return _advancePending;
}

// Actualizează faza de clipit în standby. Returnează true dacă s-a schimbat
// starea vizibilă a indicatorului (aprins/stins).
bool BatteryCheckScreen::updateStandbyBlinkPhase()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _standbyBlinkTimerMs;

    bool newVisible = _standbyIndicatorVisible;

    if (_standbyBlinkOn) {
        if (elapsed >= STANDBY_BLINK_ON_MS) {
            _standbyBlinkOn = false;
            _standbyBlinkTimerMs = now;
            newVisible = false;
        } else {
            newVisible = true;
        }
    } else {
        if (elapsed >= STANDBY_BLINK_OFF_MS) {
            _standbyBlinkOn = true;
            _standbyBlinkTimerMs = now;
            newVisible = true;
        } else {
            newVisible = false;
        }
    }

    if (newVisible == _standbyIndicatorVisible) {
        return false;
    }
    _standbyIndicatorVisible = newVisible;
    return true;
}

void BatteryCheckScreen::drawMainContent()
{
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();

    _tft->fillScreen(COLOR_BACKGROUND);

    // Titlu: BATTERY LEVEL
    _tft->setTextColor(COLOR_TEXT_MAIN);
    _tft->setTextSize(2);

    int16_t titleWidth = 13 * 6 * 2; // "BATTERY LEVEL" = 13 caractere * 6px * textSize 2
    _tft->setCursor((240 - titleWidth) / 2, 80);
    _tft->print("BATTERY LEVEL");

    // Procent: XX %
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d %%", level);

    _tft->setTextSize(4);
    int16_t percentWidth = strlen(percentStr) * 6 * 4;
    _tft->setCursor((240 - percentWidth) / 2, 130);
    _tft->print(percentStr);

    // Stare de încărcare
    if (charging) {
        _tft->setTextColor(COLOR_YELLOW);
        _tft->setTextSize(2);
        const char* chargingText = "CHARGING";
        int16_t chargingWidth = strlen(chargingText) * 6 * 2;
        _tft->setCursor((240 - chargingWidth) / 2, 200);
        _tft->print(chargingText);
    }

    // Indicator vizual (punct) în centrul ecranului, sub text.
    drawIndicator();

    _displayedLevel = level;
    _displayedCharging = charging;
}

void BatteryCheckScreen::drawIndicator()
{
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();

    BatteryLevelState state = evaluateLevel(level);
    uint16_t color = levelStateToColor(state);

    // Dacă bateria este complet încărcată (100% + charging), indicator verde
    // continuu (fără modificări de culoare).
    if (level == 100 && charging) {
        color = COLOR_GREEN;
    }

    // Șterge zona indicatorului pentru a evita artefacte la schimbarea culorii.
    _tft->fillCircle(120, 260, INDICATOR_RADIUS + 2, COLOR_BACKGROUND);
    _tft->fillCircle(120, 260, INDICATOR_RADIUS, color);
}

void BatteryCheckScreen::drawStandbyContent()
{
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();

    // Șterge doar zona indicatorului pentru a evita artefacte.
    _tft->fillCircle(120, 260, INDICATOR_RADIUS + 2, COLOR_BACKGROUND);

    if (_standbyIndicatorVisible) {
        BatteryLevelState state = evaluateLevel(level);
        uint16_t color = levelStateToColor(state);
        if (level == 100 && charging) {
            color = COLOR_GREEN;
        }
        _tft->fillCircle(120, 260, INDICATOR_RADIUS, color);
    }

    // Procent permanent aprins, deasupra indicatorului, textSize 1, culoare muted.
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d %%", level);

    // Înălțime font textSize 1 = 8 px; zona text 16 px înălțime pentru curățare.
    const int textY = 240;
    _tft->fillRect(80, textY - 2, 80, 14, COLOR_BACKGROUND);

    _tft->setTextColor(COLOR_TEXT_MUTED);
    _tft->setTextSize(1);
    int16_t textWidth = strlen(percentStr) * 6;
    _tft->setCursor((240 - textWidth) / 2, textY);
    _tft->print(percentStr);
}

void BatteryCheckScreen::clearScreenKeepIndicator()
{
    _tft->fillScreen(COLOR_BACKGROUND);
    _standbyBlinkTimerMs = millis();
    _standbyBlinkOn = true;
    _standbyIndicatorVisible = true; // punctul pornește aprins, fără cadru negru
    drawStandbyContent();
}

BatteryCheckScreen::BatteryLevelState BatteryCheckScreen::evaluateLevel(uint8_t percent) const
{
    if (percent < LOW_THRESHOLD_PERCENT) {
        return BatteryLevelState::BAT_LOW;
    }
    if (percent < MEDIUM_THRESHOLD_PERCENT) {
        return BatteryLevelState::BAT_MEDIUM;
    }
    return BatteryLevelState::BAT_GOOD;
}

uint16_t BatteryCheckScreen::levelStateToColor(BatteryLevelState state) const
{
    switch (state) {
        case BatteryLevelState::BAT_LOW:
            return COLOR_RED;
        case BatteryLevelState::BAT_MEDIUM:
            return COLOR_YELLOW;
        case BatteryLevelState::BAT_GOOD:
        default:
            return COLOR_GREEN;
    }
}
