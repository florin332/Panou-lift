#include "StartupScreen.h"

// Unified Color Palette (Hex conversion for ST7789/ILI9341 16-bit RGB565)
#define COLOR_BACKGROUND 0x0000 // Black
#define COLOR_TEXT_MAIN  0xFFFF // White
#define COLOR_TEXT_MUTED 0x9DF3 // Gray
#define COLOR_BTN_BASE   0x1967 // Deep Blue
#define COLOR_BTN_PUSH   0x341F // Lighter Blue/Cyan accent
#define COLOR_GREEN      0x07E0 // Bright Green
#define COLOR_RED        0xF800 // Red

StartupScreen::StartupScreen(Adafruit_GFX* tftInstance, IBatteryProvider& battery)
    : _tft(tftInstance)
    , _battery(battery)
    , _isRendered(false)
    , _buttonPressed(false)
    , _displayedLevel(0xFF)
    , _displayedCharging(false)
    , _lastBatteryUpdateMs(0)
{
}

void StartupScreen::init() {
    _isRendered = false;
    _buttonPressed = false;
    _displayedLevel = 0xFF;
    _displayedCharging = false;
    _lastBatteryUpdateMs = 0;
}

void StartupScreen::render(bool forceRedraw) {
    if (_isRendered && !forceRedraw) return;

    _tft->fillScreen(COLOR_BACKGROUND);

    // Title Blocks (Manually centered on 240px width screen)
    _tft->setTextColor(COLOR_TEXT_MAIN);
    _tft->setTextSize(3);
    _tft->setCursor(25, 45);
    _tft->print("SERVICE BOX");

    _tft->setTextSize(2);
    _tft->setCursor(16, 95);
    _tft->print("PANOURI ASCENSOARE");

    // Version Tag
    _tft->setTextColor(COLOR_TEXT_MUTED);
    _tft->setTextSize(1);
    _tft->setCursor(102, 135);
    _tft->print("v1.0.0");

    // Base Interactive Button Render
    drawButton(false);

    // Battery indicator (above SD status, same style as BatteryCheckScreen)
    drawBatteryInfo();

    // Default Storage Row (Will override on runtime update calls)
    updateStorageInfo(0, 0, false);

    _isRendered = true;
}

void StartupScreen::drawButton(bool pressed) {
    uint16_t btnColor = pressed ? COLOR_BTN_PUSH : COLOR_BTN_BASE;
    
    // Draw Round Box outline filled canvas
    _tft->fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, BTN_RADIUS, btnColor);
    _tft->drawRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, BTN_RADIUS, COLOR_TEXT_MAIN);

    // Center Text inside button geometries
    _tft->setTextColor(COLOR_TEXT_MAIN);
    _tft->setTextSize(2);
    _tft->setCursor(BTN_X + 41, BTN_Y + 15);
    _tft->print("START");
}

void StartupScreen::drawBatteryInfo() {
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();

    // Clear battery display area (between button and SD status, left aligned with SD)
    _tft->fillRect(10, 266, 220, 20, COLOR_BACKGROUND);

    // Colored indicator dot (same style as SD indicator)
    uint16_t color = levelToColor(level);
    if (level == 100 && charging) {
        color = COLOR_GREEN;
    }
    _tft->fillCircle(20, 274, BATTERY_DOT_RADIUS, color);

    // Text aligned with SD status, textSize 1, fixed muted color
    char percentStr[24];
    snprintf(percentStr, sizeof(percentStr), "BATTERY LEVEL: %d %%", level);
    _tft->setTextSize(1);
    _tft->setTextColor(COLOR_TEXT_MUTED);
    _tft->setCursor(32, 271);
    _tft->print(percentStr);

    _displayedLevel = level;
    _displayedCharging = charging;
}

uint16_t StartupScreen::levelToColor(uint8_t percent) const {
    if (percent < LOW_THRESHOLD_PERCENT) {
        return COLOR_RED;
    }
    if (percent < MEDIUM_THRESHOLD_PERCENT) {
        return COLOR_YELLOW;
    }
    return COLOR_GREEN;
}

void StartupScreen::updateStorageInfo(uint32_t freeMB, uint32_t totalMB, bool cardAvailable) {
    // Overwrite bottom line canvas chunk to avoid flicker artifacts
    _tft->fillRect(10, 282, 220, 20, COLOR_BACKGROUND);
    _tft->setTextSize(1);

    if (cardAvailable) {
        // SD Status Indicator Bullet (Green dot)
        _tft->fillCircle(20, 290, 4, COLOR_GREEN);
        _tft->setTextColor(COLOR_TEXT_MUTED);
        _tft->setCursor(32, 287);
        _tft->print("SD: " + String(freeMB) + " MB / " + String((float)totalMB / 1000.0, 1) + " GB FREE");
    } else {
        // SD Disconnected Bullet (Red dot)
        _tft->fillCircle(20, 290, 4, COLOR_RED);
        _tft->setTextColor(COLOR_TEXT_MUTED);
        _tft->setCursor(32, 287);
        _tft->print("SD CARD DISCONNECTED / ERROR");
    }
}

bool StartupScreen::update(int touchX, int touchY, bool isTouched) {
    bool startTriggered = false;

    // Periodic battery update (same interval as BatteryCheckScreen)
    unsigned long now = millis();
    if (now - _lastBatteryUpdateMs >= BATTERY_UPDATE_MS) {
        _lastBatteryUpdateMs = now;
        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();
        if (currentLevel != _displayedLevel || currentCharging != _displayedCharging) {
            drawBatteryInfo();
        }
    }

    if (isTouched) {
        // Evaluate boundary box intersection parameters
        if (touchX >= BTN_X && touchX <= (BTN_X + BTN_W) &&
            touchY >= BTN_Y && touchY <= (BTN_Y + BTN_H)) {
            
            if (!_buttonPressed) {
                _buttonPressed = true;
                drawButton(true); // Redraw button in active pushed state instantly
            }
        } else {
            // Finger drifted outside boundary limits while pressing down
            if (_buttonPressed) {
                _buttonPressed = false;
                drawButton(false);
            }
        }
    } else {
        // Finger released from active capacitive/resistive target window
        if (_buttonPressed) {
            _buttonPressed = false;
            drawButton(false);
            startTriggered = true; // Dispatch execution signal on confirmation release
        }
    }

    return startTriggered;
}
