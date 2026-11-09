#include "AlarmService.h"
#include "Pins.h"
#include "Sound.h"
#include <Arduino.h>

namespace AlarmService
{
    namespace
    {
        unsigned long lastOverloadSoundMillis = 0;
        bool overloadWasActive = false;
    }

    void init() {
        Sound::init();
        lastOverloadSoundMillis = 0;
        overloadWasActive = false;
    }

    void initializeBootState(SharedPanel &state) {
        state.ui.sound.event = SoundEvent::Boot;
        state.ui.sound.seq = 1;
    }

    void update(const SharedPanel &state, Direction previousDirection) {
        if (previousDirection != Direction::Idle && state.lift1.sj == Direction::Idle) {
            Sound::liftStopped();
        }

        const bool overloadActive = digitalRead(Pins::Inputs::OVERLOAD) == LOW;
        if (!overloadActive) {
            overloadWasActive = false;
        } else if (!overloadWasActive) {
            Sound::overload();
            lastOverloadSoundMillis = millis();
            overloadWasActive = true;
        } else if (millis() - lastOverloadSoundMillis >= 5000 && !Sound::isPlaying()) {
            Sound::overload();
            lastOverloadSoundMillis = millis();
        }
    }

    bool isPlaying() { return Sound::isPlaying(); }
    void stop() { Sound::stop(); }
    bool setGain(float value) { return Sound::setGain(value); }
    float gain() { return Sound::gain(); }
}