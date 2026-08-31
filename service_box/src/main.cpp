#include <Arduino.h>
#include "hal/HardwareInterface.h"

void setup() {
    // În funcție de selectarea mediului din PlatformIO (marble_pico sau waveshare_rp2350),
    // se va rula automat funcția init() din spatele clasei potrivite.
    Hardware.init();
}

void loop() {
    // Actualizează starea coordonatelor de touch
    Hardware.updateTouch();
    
    if (Hardware.isScreenTouched()) {
        int x = Hardware.getTouchX();
        int y = Hardware.getTouchY();
        
        // Aici vom implementa logica meniului tactil de service.
        // Copilot va desena butoanele și va verifica dacă X și Y se află în interiorul lor.
    }
    
    delay(10); // Scurt delay pentru stabilitate
}
