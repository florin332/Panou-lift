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
        bool buzzerOn;
        uint16_t currentFreq;
        SoundEvent currentEvent;
    };

    static SoundContext ctx = {0, 0, false, false, 0, SoundEvent::None};

    static void setBuzzer(bool stare, uint16_t freq = Config::Audio::BUZZER_FREQ) {
        // tone()/noTone() doar la TRANZITIE (sau la schimbarea frecventei):
        // reaplelarea tone() in bucla reseteaza faza PWM si transforma
        // pulsul continuu intr-un "carait" audibil
        if (stare == ctx.buzzerOn && (!stare || freq == ctx.currentFreq)) return;
        ctx.buzzerOn = stare;
        ctx.currentFreq = freq;
        if (stare) {
            tone(Pins::UI::BUZZER, freq);
        } else {
            noTone(Pins::UI::BUZZER);
        }
    }

    void init() {
        pinMode(Pins::UI::BUZZER, OUTPUT);
        digitalWrite(Pins::UI::BUZZER, LOW);
        ctx = {0, 0, false, false, 0, SoundEvent::None};
    }

    void update(const SharedPanel &localPanel) {
        unsigned long currentMillis = millis();

        if (localPanel.ui.sound.seq != ctx.lastProcessedSeq) {
            ctx.lastProcessedSeq = localPanel.ui.sound.seq;
            
            if (localPanel.ui.sound.event != SoundEvent::None) {
                SoundEvent incoming = localPanel.ui.sound.event;
                // Fuziune: un alt Arrival in timpul unei secvente Arrival/ArrivalBoth in curs
                // comuta pe varianta extinsa (3 beep-uri), repornita de la t=0
                if (incoming == SoundEvent::Arrival && ctx.isActive &&
                    (ctx.currentEvent == SoundEvent::Arrival ||
                     ctx.currentEvent == SoundEvent::ArrivalBoth)) {
                    incoming = SoundEvent::ArrivalBoth;
                }
                ctx.currentEvent = incoming;
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
                    setBuzzer(true, Config::Audio::ARRIVAL_FIRST_FREQ);
                }
                else if (timpTrecut >= Config::Audio::ARRIVAL_FIRST_PULSE_MS && timpTrecut < Config::Audio::ARRIVAL_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut >= Config::Audio::ARRIVAL_GAP_MS && timpTrecut < Config::Audio::ARRIVAL_SECOND_PULSE_MS) {
                    setBuzzer(true, Config::Audio::ARRIVAL_SECOND_FREQ);
                }
                else {
                    setBuzzer(false); ctx.isActive = false; ctx.currentEvent = SoundEvent::None;
                }
                break;

            case SoundEvent::ArrivalBoth:
                if (timpTrecut < Config::Audio::ARRIVAL_FIRST_PULSE_MS) {
                    setBuzzer(true, Config::Audio::ARRIVAL_FIRST_FREQ);
                }
                else if (timpTrecut < Config::Audio::ARRIVAL_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut < Config::Audio::ARRIVAL_SECOND_PULSE_MS) {
                    setBuzzer(true, Config::Audio::ARRIVAL_SECOND_FREQ);
                }
                else if (timpTrecut < Config::Audio::ARRIVAL_BOTH_SECOND_GAP_MS) {
                    setBuzzer(false);
                }
                else if (timpTrecut < Config::Audio::ARRIVAL_BOTH_END_MS) {
                    setBuzzer(true, Config::Audio::ARRIVAL_FIRST_FREQ);
                }
                else {
                    setBuzzer(false); ctx.isActive = false; ctx.currentEvent = SoundEvent::None;
                }
                break;

            case SoundEvent::Free:
                if (timpTrecut < Config::Audio::FREE_PULSE_MS) {
                    setBuzzer(true, Config::Audio::FREE_FREQ);
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
