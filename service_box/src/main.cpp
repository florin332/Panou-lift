#include <Arduino.h>
#include "hal/HardwareInterface.h"

unsigned long lastDebugTime = 0;

void setup() {
    // Pornește comunicarea serială pentru debug-ul pe PC
    Serial.begin(115200);
    delay(1000); // Scurtă pauză pentru stabilizare terminal

    // Inițializează hardware-ul selectat automat la compilare (Marble sau Waveshare)
    Hardware.init();

    Serial.println("==========================================");
    Serial.print("Sistem pornit cu succes pe: ");
    Serial.println(MY_BOARD_NAME);
    Serial.println("==========================================");
}

void loop() {
    // Interoghează controlerul tactil (XPT2046 pe Marble sau CST328 pe Waveshare)
    Hardware.updateTouch();

    if (Hardware.isScreenTouched()) {
        int touchX = Hardware.getTouchX();
        int touchY = Hardware.getTouchY();
        
        // Evităm inundarea portului serial: afișăm datele o dată la 100ms în caz de apăsare continuă
        if (millis() - lastDebugTime > 100) {
            Serial.print("[TOUCH DETECTAT] X = ");
            Serial.print(touchX);
            Serial.print(" | Y = ");
            Serial.println(touchY);

            // Generăm un pachet simplu de test pentru a verifica trimiterea prin UART1 (Serial2)
            uint8_t pachetTest[4] = {0xAA, (uint8_t)(touchX >> 8), (uint8_t)(touchX & 0xFF), 0xBB};
            
            // Apelează funcția unică din HAL
            Hardware.sendProtocolData(pachetTest, sizeof(pachetTest));

            lastDebugTime = millis();
        }
    }
}


