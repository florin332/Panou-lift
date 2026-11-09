//Drivers/Sound.h (Interfața Driverului Acustic)
#pragma once

#include <cstddef>
#include <cstdint>

namespace Sound
{
    // Initialize native RP2040 I2S and validate built-in WAV clips.
    void init();

    // Feed available DMA buffers without blocking.
    void update();

    // Cross-core SD streaming interface; producer runs on Core 1.
    bool beginSdStream();
    size_t sdStreamFreeSamples();
    size_t pushSdStreamSamples(const int16_t *samples, size_t count);
    void finishSdStream();
    bool isSdStreamActive();

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
