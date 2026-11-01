// Logic/LiftController.cpp (Calcul Duplex cu Uzură și Anti-Stuck)

#include "LiftController.h" // Local în src/Logic/

// Rute către folderul src/ (Urcat un nivel)
#include "Pins.h"
#include "Config.h"
#include "Settings.h"

#include "Arduino.h"
#include <hardware/watchdog.h>


namespace LiftController
{
    struct ControllerContext {
        unsigned long startMillis;
        unsigned long buttPressedMillis;
        bool buttonStuck;
        bool b_prev;
        bool alterna_uzura;
        bool buton_a_fost_apasat;
        bool resetArmed;
        Direction prevSj1;
        Direction prevSj2;
        Occupancy prevOcp1;
        Occupancy prevOcp2;
    };

    static ControllerContext ctx;

    // --- SUBRUTINĂ PRIVATĂ 1: GESTIUNE FILTRU ANTI-STUCK ȘI RESET LA APĂSARE LUNGĂ ---
    static void updateButtonState(bool b_state, unsigned long currentMillis) {
        if (b_state) {
            if (ctx.b_prev == false) { ctx.buttPressedMillis = currentMillis; }
            if ((currentMillis - ctx.buttPressedMillis) >= Config::Timing::BUTTON_STUCK_MS) { ctx.buttonStuck = true; }

            // Reset hardware la apăsare lungă (prag SUB anti-stuck).
            // Armat doar după ce butonul a fost eliberat cel puțin o dată post-boot:
            // previne bucla infinită de reboot dacă butonul rămâne blocat fizic.
            if (ctx.resetArmed && (currentMillis - ctx.buttPressedMillis) >= Config::Timing::BUTTON_RESET_MS) {
                watchdog_hw->scratch[0] = Config::Hardware::RESET_MAGIC_BUTTON;
                // watchdog_reboot atârnă la infinit dacă tick-ul watchdog nu rulează (vezi ServiceMenu.cpp)
                watchdog_start_tick(12);
                watchdog_reboot(0, 0, 0);   // Nu returnează niciodată
            }
        } else {
            ctx.buttonStuck = false; 
            ctx.resetArmed = true;
        }
    }

    // --- SUBRUTINĂ PRIVATĂ 2: GESTIUNE COMPORTAMENT LEDURI ȘI AUDIO LOCAL ---
    static void updateLeds(SharedPanel &localPanel, bool b_state, uint8_t nocom, unsigned long currentMillis) {
        if (ctx.buttonStuck) {
            localPanel.ui.led.red   = LedMode::Off;
            localPanel.ui.led.green = LedMode::Off;
            return;
        }

        switch(nocom) {
            case 0:
                if (currentMillis - ctx.startMillis < Config::Timing::BUTTON_INHIBIT_MS && ctx.startMillis != 0) {
                    localPanel.ui.led.red   = LedMode::On; localPanel.ui.led.green = LedMode::Off;
                } else {
                    if (localPanel.lift1.ocp == Occupancy::Busy && localPanel.lift2.ocp == Occupancy::Busy) {
                        localPanel.ui.led.red   = LedMode::On; localPanel.ui.led.green = LedMode::Off;
                    } else {
                        localPanel.ui.led.red   = LedMode::Off; localPanel.ui.led.green = LedMode::On;
                    }
                }
                break;

            case 1:
                localPanel.ui.led.red   = LedMode::Off;
                localPanel.ui.led.green = LedMode::BlinkSlow;
                if (b_state && (ctx.b_prev == false)) {
                    // PROVIZORIU: sunetul Error este dezactivat
                    // localPanel.ui.sound.event = SoundEvent::Error;
                    // localPanel.ui.sound.seq++;
                }
                break;
        }
    }

    // --- SUBRUTINĂ PRIVATĂ 3: DETECȚIE SOSIRE LA PALIER (FRONT SJ -> IDLE) ---
    static void updateArrivalSound(SharedPanel &localPanel) {
        bool arrival1 = (ctx.prevSj1 != Direction::Idle) &&
                        (localPanel.lift1.sj == Direction::Idle) &&
                        (localPanel.lift1.pos == Settings::panelFloor()) &&
                        (localPanel.lift1.svc == ServiceState::Normal);
        bool arrival2 = (ctx.prevSj2 != Direction::Idle) &&
                        (localPanel.lift2.sj == Direction::Idle) &&
                        (localPanel.lift2.pos == Settings::panelFloor()) &&
                        (localPanel.lift2.svc == ServiceState::Normal);

        if (arrival1 || arrival2) {
            localPanel.ui.sound.event = SoundEvent::Arrival;
            localPanel.ui.sound.seq++;
        }

        // Al treilea beep: cabina s-a eliberat (ocp 1 -> 0) la etajul panoului, lift oprit
        bool freed1 = (ctx.prevOcp1 == Occupancy::Busy) &&
                      (localPanel.lift1.ocp == Occupancy::Free) &&
                      (localPanel.lift1.pos == Settings::panelFloor()) &&
                      (localPanel.lift1.sj == Direction::Idle) &&
                      (localPanel.lift1.svc == ServiceState::Normal);
        bool freed2 = (ctx.prevOcp2 == Occupancy::Busy) &&
                      (localPanel.lift2.ocp == Occupancy::Free) &&
                      (localPanel.lift2.pos == Settings::panelFloor()) &&
                      (localPanel.lift2.sj == Direction::Idle) &&
                      (localPanel.lift2.svc == ServiceState::Normal);

        if (freed1 || freed2) {
            localPanel.ui.sound.event = SoundEvent::Free;
            localPanel.ui.sound.seq++;
        }

        ctx.prevSj1 = localPanel.lift1.sj;
        ctx.prevSj2 = localPanel.lift2.sj;
        ctx.prevOcp1 = localPanel.lift1.ocp;
        ctx.prevOcp2 = localPanel.lift2.ocp;
    }

    // --- SUBRUTINĂ PRIVATĂ 4: ALGORITM SELECȚIE DUPLEX PRIORITARĂ ---
    static uint8_t selectLift(const SharedPanel &localPanel) {
        int dif1 = abs(int(localPanel.lift1.pos) - int(Settings::panelFloor()));
        int dif2 = abs(int(localPanel.lift2.pos) - int(Settings::panelFloor()));

        if (localPanel.lift1.ocp == Occupancy::Busy && localPanel.lift2.ocp == Occupancy::Free)       return 2;
        if (localPanel.lift1.ocp == Occupancy::Free && localPanel.lift2.ocp == Occupancy::Busy)       return 1;
        if (localPanel.lift1.ocp == Occupancy::Free && localPanel.lift2.ocp == Occupancy::Free) {
            if (dif1 < dif2) return 1;
            if (dif1 > dif2) return 2;
            return ctx.alterna_uzura ? 2 : 1;
        }
        return 0;
    }

    // --- SUBRUTINĂ PRIVATĂ 5: LOGICĂ DE ROTAȚIE ȘI BALANSARE UZURĂ ---
    static void updateWearBalancer(const SharedPanel &localPanel) {
        if ((localPanel.lift1.etd == Settings::panelFloor() || localPanel.lift2.etd == Settings::panelFloor()) && !ctx.buton_a_fost_apasat) {
            ctx.alterna_uzura = !ctx.alterna_uzura; ctx.buton_a_fost_apasat = true;
        }
        if (localPanel.lift1.etd != Settings::panelFloor() && localPanel.lift2.etd != Settings::panelFloor()) {
            ctx.buton_a_fost_apasat = false;
        }
    }

    // --- SUBRUTINĂ PRIVATĂ 6: INTERCEPTARE ȘI PROCESARE APĂSARE VALIDĂ ---
    static bool processCall(unsigned long currentMillis, uint8_t nocom, uint8_t selectedAsc) {
        if (nocom == 0 && selectedAsc != 0) {
            if (currentMillis - ctx.startMillis >= Config::Timing::BUTTON_INHIBIT_MS) {
                ctx.startMillis = currentMillis;
                return true;
            }
        }
        return false;
    }

    void init() {
        pinMode(Pins::UI::BUTTON, INPUT_PULLUP);
        ctx.startMillis = 0;
        ctx.buttPressedMillis = 0;
        ctx.buttonStuck = false;
        ctx.b_prev = true; 
        ctx.alterna_uzura = false;
        ctx.buton_a_fost_apasat = false;
        // Reset-ul pe apăsare lungă se armează doar dacă butonul e eliberat la boot
        ctx.resetArmed = (digitalRead(Pins::UI::BUTTON) == HIGH);
        // Idle la boot: un lift parcat la etajul panoului nu declanșează Arrival la pornire
        ctx.prevSj1 = Direction::Idle;
        ctx.prevSj2 = Direction::Idle;
        // Free la boot: evita un beep fals la prima eliberare raportata dupa pornire
        ctx.prevOcp1 = Occupancy::Free;
        ctx.prevOcp2 = Occupancy::Free;
    }

    // INTERFAȚA PUBLICĂ ORCHESTRATOARE COMPLET DECUPLATĂ
    ControllerResult process(SharedPanel &localPanel) {
        unsigned long currentMillis = millis();
        bool b_state = (digitalRead(Pins::UI::BUTTON) == LOW);
        
        ControllerResult result = {false, 0, false, false, false, false};

        if (localPanel.lift1.svc != ServiceState::Normal) { localPanel.lift1.ocp = Occupancy::Busy; }
        if (localPanel.lift2.svc != ServiceState::Normal) { localPanel.lift2.ocp = Occupancy::Busy; }

        // Executarea liniară a subrutinelor private (Observația 5)
        updateButtonState(b_state, currentMillis);
        
        uint8_t nocom = ((localPanel.lift1.pos == Settings::panelFloor() && localPanel.lift1.sj == Direction::Idle) || 
                         (localPanel.lift2.pos == Settings::panelFloor() && localPanel.lift2.sj == Direction::Idle)) ? 1 : 0;
        
        updateLeds(localPanel, b_state, nocom, currentMillis);
        
        if (ctx.buttonStuck) {
            ctx.b_prev = b_state;
            return result;
        }

        uint8_t selectedAsc = selectLift(localPanel);
        updateWearBalancer(localPanel);
        updateArrivalSound(localPanel);

        if (b_state && (ctx.b_prev == false)) {
            if (processCall(currentMillis, nocom, selectedAsc)) {
                result.transmit = true;
                result.lift = selectedAsc;
                localPanel.ui.sound.event = SoundEvent::Confirm; 
                localPanel.ui.sound.seq++;
                localPanel.ui.led.red = LedMode::BlinkFast;
            }
        }

        ctx.b_prev = b_state;
        return result;
    }
}
