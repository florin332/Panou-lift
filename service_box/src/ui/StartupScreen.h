#ifndef STARTUP_SCREEN_H
#define STARTUP_SCREEN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "hal/HardwareInterface.h"

class StartupScreen {
private:
    Adafruit_GFX* _tft;
    bool _isRendered;
    bool _buttonPressed;
    
    // UI Geometry Constants (Portrait 240x320)
    const int BTN_X = 50;
    const int BTN_Y = 190;
    const int BTN_W = 140;
    const int BTN_H = 45;
    const int BTN_RADIUS = 6;

    void drawButton(bool pressed);

public:
    StartupScreen(Adafruit_GFX* tftInstance);
    void init();
    void render(bool forceRedraw = false);
    bool update(int touchX, int touchY, bool isTouched); // Returns true when START is pressed & released
    void updateStorageInfo(uint32_t freeMB, uint32_t totalMB, bool cardAvailable);
};

#endif // STARTUP_SCREEN_H
