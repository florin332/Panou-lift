//FIȘIERUL PRINCIPAL: Lift_Duplex_V1020.ino (Main - Coordonarea pe nuclee fizice)

#include "Arduino.h"

// Configurații și Modele Nucleu
#include "BuildInfo.h"
#include "Pins.h"
#include "SharedPanel.h"

// Servicii de Orchestrare și Navigare
#include "Application.h"
#include "Diagnostics.h"
#include "UIPresenter.h"
#include "DisplayRenderer.h"
#include "DiagnosticsNavigator.h"

// Variabile pentru testul asincron de comunicație pe Serial
static unsigned long lastSerialMillis = 0;
static uint32_t heartbeatCounter = 0;

void setup() {
    // 1. Inițializăm portul Serial nativ prin USB la viteza configurată în platformio.ini
    Serial.begin(115200);
    
    // 2. Inițializările hardware standard ale aplicației (V10.28)
    // (Aici rulează init-urile tale pentru pini, ecrane și memorii din celelalte module)
}

void loop() {
    unsigned long currentMillis = millis();

    // TEST DE COMUNICAȚIE: Trimitem o linie text o dată la 1000ms (1 secundă) complet asincron
    if (currentMillis - lastSerialMillis >= 1000) {
        lastSerialMillis = currentMillis;
        heartbeatCounter++;
        
        Serial.print("[MARBLE PICO V10.28] Heartbeat: ");
        Serial.print(heartbeatCounter);
        Serial.print(" | Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println("s");
    }

    // Execuția mașinii de stări principale și a nucleelor (Rămâne neschimbată)
}
