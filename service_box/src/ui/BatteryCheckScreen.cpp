#include "BatteryCheckScreen.h"

BatteryCheckScreen::BatteryCheckScreen(Adafruit_GFX* tft, IBatteryProvider& battery)
    : _tft(tft)
    , _battery(battery)
    , _isRendered(false)
    , _screenOn(true)
    , _advancePending(false)
    , _lastActivityMs(0)
    , _lastBatteryUpdateMs(0)
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
        // Stare stinsă: doar punct indicator, fără re-redare continuă.
        // Dacă s-a schimbat nivelul, re-desenăm indicatorul.
        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();
        if (currentLevel != _displayedLevel || currentCharging != _displayedCharging) {
            drawIndicator();
            _displayedLevel = currentLevel;
            _displayedCharging = currentCharging;
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

            // Dacă ecranul este stins, re-desenăm indicatorul imediat.
            if (!_screenOn) {
                drawIndicator();
                _displayedLevel = currentLevel;
                _displayedCharging = currentCharging;
            }
        }
    }

    // Timeout de 5 secunde fără atingere.
    if (_screenOn && (now - _lastActivityMs >= SCREEN_ON_TIMEOUT_MS)) {
        BatteryLevelState levelState = evaluateLevel(_battery.getLevelPercent());
        bool charging = _battery.isCharging();

        if (charging || levelState == BatteryLevelState::LOW) {
            // LOW sau CHARGING: rămâne în Battery Check, doar stinge ecranul.
            _screenOn = false;
            _isRendered = false;
            clearScreenKeepIndicator();
        }
        else {
            // MEDIUM/GOOD: avansare automată către START.
            _advancePending = true;
        }
    }

    return wasReactivated;
}

bool BatteryCheckScreen::shouldAdvance() const
{
    return _advancePending;
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

void BatteryCheckScreen::clearScreenKeepIndicator()
{
    _tft->fillScreen(COLOR_BACKGROUND);
    drawIndicator();
}

BatteryLevelState BatteryCheckScreen::evaluateLevel(uint8_t percent) const
{
    if (percent < LOW_THRESHOLD_PERCENT) {
        return BatteryLevelState::LOW;
    }
    if (percent < MEDIUM_THRESHOLD_PERCENT) {
        return BatteryLevelState::MEDIUM;
    }
    return BatteryLevelState::GOOD;
}

uint16_t BatteryCheckScreen::levelStateToColor(BatteryLevelState state) const
{
    switch (state) {
        case BatteryLevelState::LOW:
            return COLOR_RED;
        case BatteryLevelState::MEDIUM:
            return COLOR_YELLOW;
        case BatteryLevelState::GOOD:
        default:
            return COLOR_GREEN;
    }
}
