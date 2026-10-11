//Drivers/Sound.h (Interfața Driverului Acustic)
#pragma once

#include "SharedPanel.h"

namespace Sound
{
    // Initialize audio hardware and ESP8266Audio
    void init();

    // Non-blocking audio processing
    void update(const SharedPanel &localPanel);

    // Play warning sounds
    void alarm();
    void overload();
    void liftStopped();

    // Stop currently playing sound
    void stop();

    // Returns true while a sound is being played
    bool isPlaying();

    // Gain persistent (Settings / flash protejat).
    // setGain aplică imediat pe ieșirea I2S și salvează în flash.
    bool setGain(float gain);
    float gain();
}
