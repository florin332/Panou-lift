// Logic/LiftController.cpp (Calcul Duplex cu Uzură, Anti-Stuck și Confirmare Preluare Apel)

#include "LiftController.h" // Local în src/Logic/

// Rute către folderul src/ (Urcat un nivel)
#include "Pins.h"
#include "Config.h"
#include "Settings.h"

#include "Arduino.h"
#include <hardware/watchdog.h>


namespace LiftController
{
    // Mașina de stări pentru confirmarea preluării apelului:
    // Idle -> Sent (comandă trimisă, așteaptă tranziția etd) -> Confirmed (cursă în execuție)
    enum class CallState : uint8_t { Idle, Sent, Confirmed };

    struct ControllerContext {
        unsigned long startMillis;
        unsigned long buttPressedMillis;
        bool buttonStuck;
        bool b_prev;
        bool alterna_uzura;
        bool buton_a_fost_apasat;
        uint8_t resetPressCount;             // apăsări consecutive acumulate în fereastra de reset
        unsigned long firstResetPressMillis; // începutul ferestrei de reset (prima apăsare din serie)
        Direction prevSj1;
        Direction prevSj2;
        Occupancy prevOcp1;
        Occupancy prevOcp2;
        // Confirmare preluare apel
        CallState callState;
        uint8_t cmdLift;                // liftul comandat (1/2)
        unsigned long callStartMillis;  // momentul trimiterii comenzii (timeout unic)
        bool runStarted;                // latch: sj a fost non-Idle după confirmare
        bool ocpArmed;                  // latch: ocp a fost Busy după confirmare
        uint8_t prevEtd1;               // bază pentru detecția tranziției etd -> etaj panou
        uint8_t prevEtd2;
        bool longPressFired;            // one-shot: al doilea apel trimis deja pe ținerea curentă
    };

    static ControllerContext ctx;

    // --- SUBRUTINĂ PRIVATĂ 1: GESTIUNE FILTRU ANTI-STUCK ȘI RESET LA 4 APĂSĂRI RAPIDE ---
    static void updateButtonState(bool b_state, unsigned long currentMillis) {
        if (b_state) {
            if (ctx.b_prev == false) {
                ctx.buttPressedMillis = currentMillis;

                // Reset hardware: 4 apăsări consecutive, toate într-o fereastră de 2 s.
                // Fereastra pornește la prima apăsare din serie; o pauză mai mare de
                // 2 s între apăsări resetează numărătorul. Un buton blocat fizic nu
                // poate produce fronturi, deci nu mai e nevoie de armare post-boot.
                if (ctx.resetPressCount == 0 ||
                    (currentMillis - ctx.firstResetPressMillis) >= Config::Timing::RESET_PRESS_WINDOW_MS) {
                    ctx.resetPressCount = 1;
                    ctx.firstResetPressMillis = currentMillis;
                } else {
                    ctx.resetPressCount++;
                    if (ctx.resetPressCount >= Config::Timing::RESET_PRESS_COUNT) {
                        watchdog_hw->scratch[0] = Config::Hardware::RESET_MAGIC_BUTTON;
                        // watchdog_reboot atârnă la infinit dacă tick-ul watchdog nu rulează (vezi ServiceMenu.cpp)
                        watchdog_start_tick(12);
                        watchdog_reboot(0, 0, 0);   // Nu returnează niciodată
                    }
                }
            }
            if ((currentMillis - ctx.buttPressedMillis) >= Config::Timing::BUTTON_STUCK_MS) { ctx.buttonStuck = true; }
        } else {
            ctx.buttonStuck = false;
        }
    }

    // --- SUBRUTINĂ PRIVATĂ 2: GESTIUNE COMPORTAMENT LEDURI ȘI AUDIO LOCAL ---
    static void updateLeds(SharedPanel &localPanel, bool b_state, uint8_t nocom, unsigned long currentMillis) {
        if (ctx.buttonStuck) {
            localPanel.ui.led.red   = LedMode::Off;
            localPanel.ui.led.green = LedMode::Off;
            return;
        }

        // Apel preluat și în execuție: blink alternativ roșu/verde cât butonul e blocat
        if (ctx.callState == CallState::Confirmed) {
            localPanel.ui.led.red   = LedMode::Alternate;
            localPanel.ui.led.green = LedMode::Alternate;
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

    // --- SUBRUTINĂ PRIVATĂ 7: MAȘINA DE STARI PENTRU CONFIRMAREA PRELUĂRII APELULUI ---
    static void updateCallState(SharedPanel &localPanel, unsigned long currentMillis) {
        if (ctx.callState != CallState::Idle) {
            const LiftState &cmd = (ctx.cmdLift == 1) ? localPanel.lift1 : localPanel.lift2;
            uint8_t prevEtd = (ctx.cmdLift == 1) ? ctx.prevEtd1 : ctx.prevEtd2;

            if (currentMillis - ctx.callStartMillis >= Config::Timing::CALL_TIMEOUT_MS) {
                // Timeout unic de la trimitere: acoperă și confirmarea absentă, și cursa neterminată
                ctx.callState = CallState::Idle;
            } else if (cmd.svc != ServiceState::Normal) {
                // Linie căzută, fault sau revizie: comanda nu mai poate fi onorată
                ctx.callState = CallState::Idle;
            } else if (ctx.callState == CallState::Sent) {
                // Confirmarea reală a preluării: etd DEVINE etajul panoului după trimitere.
                // Dacă etd era deja egal (comandă din cabină), nu există tranziție și
                // butonul nu se blochează.
                if (prevEtd != Settings::panelFloor() && cmd.etd == Settings::panelFloor()) {
                    ctx.callState = CallState::Confirmed;
                    ctx.runStarted = false;
                    ctx.ocpArmed = false;
                    localPanel.ui.sound.event = SoundEvent::Confirm;
                    localPanel.ui.sound.seq++;
                }
            } else { // Confirmed
                // sj -> Idle și ocp -> Free deblochează doar după ce cursa a pornit efectiv:
                // imediat după confirmare liftul poate sta încă pe loc (uși în curs de
                // închidere), cu sj == Idle și ocp == Free, fără ca comanda să fie abandonată
                if (cmd.sj != Direction::Idle) { ctx.runStarted = true; }
                if (cmd.ocp == Occupancy::Busy) { ctx.ocpArmed = true; }
                // ocp -> Free după start = comandă abandonată; sj -> Idle după start = sosire/oprire
                if ((ctx.ocpArmed && cmd.ocp == Occupancy::Free) ||
                    (ctx.runStarted && cmd.sj == Direction::Idle)) {
                    ctx.callState = CallState::Idle;
                }
            }
        }

        // Baza pentru detecția tranziției etd, mereu proaspătă
        ctx.prevEtd1 = localPanel.lift1.etd;
        ctx.prevEtd2 = localPanel.lift2.etd;
    }

    void init() {
        pinMode(Pins::UI::BUTTON, INPUT_PULLUP);
        ctx.startMillis = 0;
        ctx.buttPressedMillis = 0;
        ctx.buttonStuck = false;
        ctx.b_prev = true; 
        ctx.alterna_uzura = false;
        ctx.buton_a_fost_apasat = false;
        // Numărătorul de apăsări pentru reset pornește gol
        ctx.resetPressCount = 0;
        ctx.firstResetPressMillis = 0;
        // Idle la boot: un lift parcat la etajul panoului nu declanșează Arrival la pornire
        ctx.prevSj1 = Direction::Idle;
        ctx.prevSj2 = Direction::Idle;
        // Free la boot: evita un beep fals la prima eliberare raportata dupa pornire
        ctx.prevOcp1 = Occupancy::Free;
        ctx.prevOcp2 = Occupancy::Free;
        // Mașina de stări apel pornește dezarmată
        ctx.callState = CallState::Idle;
        ctx.cmdLift = 0;
        ctx.callStartMillis = 0;
        ctx.runStarted = false;
        ctx.ocpArmed = false;
        ctx.prevEtd1 = 0;
        ctx.prevEtd2 = 0;
        ctx.longPressFired = false;
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
        updateCallState(localPanel, currentMillis);

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

        // Butonul e blocat doar în Confirmed (apel preluat, în execuție). În Sent rămâne
        // activ (limitat de debounce): dacă etd era deja egal cu etajul panoului (comandă
        // din cabină), preluarea nu se confirmă și se poate relansa un apel nou.
        // Beep-ul Confirm s-a mutat pe confirmarea reală (vezi updateCallState).
        if (b_state && (ctx.b_prev == false) && ctx.callState != CallState::Confirmed) {
            if (processCall(currentMillis, nocom, selectedAsc)) {
                result.transmit = true;
                result.lift = selectedAsc;
                ctx.callState = CallState::Sent;
                ctx.cmdLift = selectedAsc;
                ctx.callStartMillis = currentMillis;
                ctx.runStarted = false;
                ctx.ocpArmed = false;
            }
        }

        // Al doilea apel (permis din Service Box, mb_13): în Confirmed, o apăsare lungă
        // (3 s) comandă și celălalt lift. selectLift rutează natural (liftul în execuție
        // e Busy); dacă ambele sunt ocupate sau unul e deja la etaj (nocom), nu se trimite.
        // One-shot pe ținere, fără semnal la trigger; tracking-ul comută pe al doilea lift.
        if (b_state && ctx.callState == CallState::Confirmed && Settings::secondCall() &&
            !ctx.longPressFired &&
            (currentMillis - ctx.buttPressedMillis) >= Config::Timing::LONG_PRESS_SECOND_CALL_MS) {
            ctx.longPressFired = true;
            uint8_t secondAsc = selectLift(localPanel);
            if (nocom == 0 && secondAsc != 0) {
                result.transmit = true;
                result.lift = secondAsc;
                ctx.callState = CallState::Sent;
                ctx.cmdLift = secondAsc;
                ctx.callStartMillis = currentMillis;
                ctx.runStarted = false;
                ctx.ocpArmed = false;
            }
        }
        if (!b_state) { ctx.longPressFired = false; }

        ctx.b_prev = b_state;
        return result;
    }
}
