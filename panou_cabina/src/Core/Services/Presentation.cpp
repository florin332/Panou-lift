//Presentation.cpp

#include "Presentation.h" // Local în src/Logic/

#include "../Drivers/Graphics.h"
#include "Arduino.h"


namespace Presentation
{
    void init() {
        Graphics::init();
    }

    bool update(const SharedPanel &snapshot) {
        const bool displayWoke = Graphics::update(snapshot);
        return displayWoke;
    }
}
