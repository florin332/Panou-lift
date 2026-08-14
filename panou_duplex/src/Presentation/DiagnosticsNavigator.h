#ifndef DIAGNOSTICS_NAVIGATOR_H
#define DIAGNOSTICS_NAVIGATOR_H

#include "DiagnosticsPages.h" // Recunoaște corect profilurile și dimensiunile

namespace DiagnosticsNavigator
{
    struct NavigatorState {
        uint8_t servicePageIndex;   
        uint8_t developerPageIndex; 
        bool isMenuOpen;            
        DiagnosticsProfile currentProfile;
    };

    void init();
    void updateButtons();
    const NavigatorState& getState();
}

#endif // DIAGNOSTICS_NAVIGATOR_H
