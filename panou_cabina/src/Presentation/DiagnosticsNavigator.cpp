#include "DiagnosticsNavigator.h"
#include "Arduino.h"



namespace DiagnosticsNavigator
{
    static NavigatorState state;
    void init() {
        state.servicePageIndex = 0;
        state.developerPageIndex = 0;
        state.isMenuOpen = false;
        state.currentProfile = DiagnosticsProfile::Service;
    }

    const NavigatorState& getState() {
        return state;
    }

    void updateButtons() {
    }
}
