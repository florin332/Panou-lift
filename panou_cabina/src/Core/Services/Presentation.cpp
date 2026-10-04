//Presentation.cpp

#include "Presentation.h" // Local în src/Logic/

#include "../Drivers/Display.h"
#include "Arduino.h"


namespace Presentation
{
    void init() {
        Display::init();
    }

    bool update(const SharedPanel &snapshot) {
        const bool displayWoke = Display::update(snapshot);
        return displayWoke;
    }
}
