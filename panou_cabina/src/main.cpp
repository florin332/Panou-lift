// FIȘIERUL PRINCIPAL: Lift_Duplex_V1029.ino (Main - Coordonarea pe nuclee fizice)

#include "Arduino.h"

#include "Application.h"

void setup() {
    Serial.begin(115200);
    Application::init();
}

void loop() {
    Application::runCore1();
    Application::runCore0();
}