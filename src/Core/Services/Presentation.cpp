//Presentation.cpp

#include "Presentation.h" // Local în src/Logic/

// 1. Rute către folderul src/Drivers/ (Urcat un nivel și intrat în Drivers)
#include "../Drivers/Display.h"
#include "../Drivers/Sound.h"
#include "../Drivers/Leds.h"

// 2. Rute către folderul src/ (Urcat un nivel și intrat în core)
#include "Config.h"

#include "Arduino.h"


namespace Presentation
{
    void init() {
        Display::init();
        Sound::init();
        Leds::init();
    }

    void update(const SharedPanel &snapshot) {
        Display::update(snapshot);
        Sound::update(snapshot);
        Leds::update(snapshot);
    }
}
