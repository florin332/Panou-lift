#include "DiagnosticsNavigator.h"
#include "DiagnosticsPages.h"   // Oferă vizibilitatea constantelor de pagini TOTAL și a profilelor
#include "Pins.h"          // Ruta simplificată PlatformIO către pinii GP16 / GP26
#include "Arduino.h"



namespace DiagnosticsNavigator
{
    static NavigatorState state;
    static unsigned long lastBtnSvcMillis = 0;
    static unsigned long lastBtnDevMillis = 0;
    static bool prevSvcState = true;
    static bool prevDevState = true;

    void init() {
        pinMode(Pins::UI::BUTTON_SVC, INPUT_PULLUP);
        pinMode(Pins::UI::BUTTON_DEV, INPUT_PULLUP);
        state.servicePageIndex = 0;
        state.developerPageIndex = 0;
        state.isMenuOpen = false;
        state.currentProfile = DiagnosticsProfile::Service;
        lastBtnSvcMillis = 0;
        lastBtnDevMillis = 0;
        prevSvcState = true;
        prevDevState = true;
    }

    const NavigatorState& getState() {
        return state;
    }

    void updateButtons() {
        unsigned long currentMillis = millis();
        
        bool currentSvc = (digitalRead(Pins::UI::BUTTON_SVC) == LOW);
        bool currentDev = (digitalRead(Pins::UI::BUTTON_DEV) == LOW);

        // --- 1. BUTON SERVICE (GP16) ---
        if (currentSvc && !prevSvcState) { 
            if (currentMillis - lastBtnSvcMillis >= 200) { 
                lastBtnSvcMillis = currentMillis;
                
                if (!state.isMenuOpen || state.currentProfile != DiagnosticsProfile::Service) {
                    state.currentProfile = DiagnosticsProfile::Service;
                    state.isMenuOpen = true;
                } else {
                    state.servicePageIndex = (state.servicePageIndex + 1) % DiagnosticsPages::TOTAL_SERVICE_PAGES;
                }
            }
        }

        // --- 2. BUTON DEVELOPER (GP26) ---
        if (currentDev && !prevDevState) {
            if (currentMillis - lastBtnDevMillis >= 200) {
                lastBtnDevMillis = currentMillis;
                
                if (!state.isMenuOpen || state.currentProfile != DiagnosticsProfile::Developer) {
                    state.currentProfile = DiagnosticsProfile::Developer;
                    state.isMenuOpen = true;
                } else {
                    state.developerPageIndex = (state.developerPageIndex + 1) % DiagnosticsPages::TOTAL_DEV_PAGES;
                }
            }
        }

        prevSvcState = currentSvc;
        prevDevState = currentDev;
    }
}
