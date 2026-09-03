//Drivers/Sound.cpp (Implementarea Mașinii de Stări Acustice Non-Blocking)


#include "Sound.h"
#include "Pins.h"

#include "Config.h"

#include "Arduino.h"

namespace Sound
{
    struct SoundContext {
        uint8_t lastProcessedSeq;
        unsigned long startMillis;
        bool isActive;
        SoundEvent currentEvent;
    };

    static SoundContext ctx = {0, 0, false, SoundEvent::None};

    static void setBuzzer(bool stare) {
        if (stare) {
            tone(Pins::UI::BUZZER, Config::Audio::BUZZER_FREQ);
        } else {
            noTone(Pins::UI::BUZZER);
        }
    }

    void init() {
        pinMode(Pins::UI::BUZZER, OUTPUT);
        digitalWrite(Pins::UI::BUZZER, LOW);
        ctx = {0, 0, false, SoundEvent::None};
    }

    void update(const SharedPanel &localPanel) {
        unsigned long currentMillis = millis();

        if (localPanel.ui.sound.seq != ctx.lastProcessedSeq) {
            ctx.lastProcessedSeq = localPanel.ui.sound.seq;
            
            if (localPanel.ui.sound.event != SoundEvent::None) {
                ctx.currentEvent = localPanel.ui.sound.event;
                ctx.startMillis = currentMillis;
                ctx.isActive = true;
                setBuzzer(true);
            } else {
                setBuzzer(false);
                ctx.isActive = false;
                ctx.currentEvent = SoundEvent::None;
            }
        }

        if (!ctx.isActive) return;
        unsigned long timpTrecut = currentMillis - ctx.startMillis;

        switch (ctx.currentEvent) {
            case SoundEvent::Boot:
            case SoundEvent::Confirm:
                if (timpTrecut >= Config::Audio::BOOT_BEEP_MS) {
                    setBuzzer(false); ctx.isActive = false; ctx.currentEvent = SoundEvent::None;
                }
                break;

            case SoundEvent::Arrival:
                if (timpTrecut < Config::Audio::ARRIVAL_FIRST_PULSE_MS) {
                    setBuzzer(true);
                }
                else if (timpTrecut >= Config::Audio::ARRIVAL_FIRST_PULSE_MS && timpTrecut < Config::Audio::ARRIVAL_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut >= Config::Audio::ARRIVAL_GAP_MS && timpTrecut < Config::Audio::ARRIVAL_SECOND_PULSE_MS) {
                    setBuzzer(true);
                }
                else {
                    setBuzzer(false); ctx.isActive = false; ctx.currentEvent = SoundEvent::None;
                }
                break;

            case SoundEvent::Error:
                if (timpTrecut < Config::Audio::ALERT_FIRST_PULSE_MS) {
                    setBuzzer(true);
                }
                else if (timpTrecut >= Config::Audio::ALERT_FIRST_PULSE_MS && timpTrecut < Config::Audio::ALERT_FIRST_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut >= Config::Audio::ALERT_FIRST_GAP_MS && timpTrecut < Config::Audio::ALERT_SECOND_PULSE_MS) {
                    setBuzzer(true);
                }
                else if (timpTrecut >= Config::Audio::ALERT_SECOND_PULSE_MS && timpTrecut < Config::Audio::ALERT_SECOND_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut >= Config::Audio::ALERT_SECOND_GAP_MS && timpTrecut < Config::Audio::ALERT_THIRD_PULSE_MS) {
                    setBuzzer(true);
                }
                else {
                    setBuzzer(false); ctx.isActive = false; ctx.currentEvent = SoundEvent::None;
                }
                break;

            default:
                break;
        }
    }
}
