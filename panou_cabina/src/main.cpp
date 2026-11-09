// FIȘIERUL PRINCIPAL: Lift_Duplex_V1029.ino (Main - Coordonarea pe nuclee fizice)

#include "Arduino.h"

#include "Application.h"

void setup() {
    Serial.begin(115200);
    Application::init();
}

void setup1() {
    Application::initCore1();
}

void loop() {
    Application::runCore0();
}

void loop1() {
    Application::runCore1();
}