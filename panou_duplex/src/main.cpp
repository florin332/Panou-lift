// FIȘIERUL PRINCIPAL: Lift_Duplex_V1029.ino (Main - Coordonarea pe nuclee fizice)

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
#include "Display.h"

// NOU: Service Mode
#include "ServiceProtocol.h"
#include "ServiceMenu.h"

void setup() {
    Serial.begin(115200);
    Application::init();
    ServiceProtocol::init();   // ← NOU
    ServiceMenu::init();       // ← NOU
}

void loop() {
    Application::runCore1();
    Application::runCore0();
}