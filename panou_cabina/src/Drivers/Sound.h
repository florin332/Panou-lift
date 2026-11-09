//Drivers/Sound.h (Interfața Driverului Acustic)
#pragma once

namespace Sound
{
    // Initialize native RP2040 I2S and validate built-in WAV clips.
    void init();

    // Feed available DMA buffers without blocking.
    void update();

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
