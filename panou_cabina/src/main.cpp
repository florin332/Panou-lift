// FIȘIERUL PRINCIPAL: Lift_Duplex_V1029.ino (Main - Coordonarea pe nuclee fizice)

#include "Arduino.h"

#include "Application.h"
#include "Settings.h"

void setup() {
    Serial.begin(115200);
    Settings::init();
    Application::init();
}

void loop() {
    Application::runCore1();
    Application::runCore0();
}