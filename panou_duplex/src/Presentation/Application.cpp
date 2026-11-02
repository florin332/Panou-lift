// src/Presentation/Application.cpp (Orchestratorul General cu Persistență Flash + Service Mode)

#include "Arduino.h"
#include "Application.h"

// 1. Rute către folderul src/ (Urcat un nivel)
#include "SharedPanel.h"
#include "BuildInfo.h"
#include "Pins.h"
#include "Config.h"

// 2. Rute interne în cadrul aceluiași folder src/Presentation/
#include "Presentation.h"
#include "Protocol.h"
#include "LiftController.h"

// 3. Rute către folderul src/Drivers/ (Urcat un nivel și intrat în Drivers)
#include "../Drivers/Display.h"

// 4. Rute către tab-urile rămase în rădăcina mare a proiectului (Urcat două niveluri)
#include "Diagnostics.h"
#include "DiagnosticsNavigator.h"
#include "UIPresenter.h"
#include "DisplayRenderer.h"
#include "PanelRenderer.h"

// 5. NOU: Service Mode — rute către src/Core/Services/ și src/Presentation/
#include "../Core/Services/ServiceProtocol.h"
#include "../Core/Services/Settings.h"
#include "ServiceMenu.h"

#include "SystemCounters.h"
#include <hardware/regs/watchdog.h>
#include "hardware/structs/vreg_and_chip_reset.h"
#include <hardware/watchdog.h>

namespace Application
{
    static uint8_t getHardwareResetReason() {
        const uint32_t watchdogReason = watchdog_hw->reason;
        const uint32_t chipResetReason = vreg_and_chip_reset_hw->chip_reset;
        vreg_and_chip_reset_hw->chip_reset =
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS |
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_BITS |
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_BITS;

        // Marca software are prioritate: scratch-ul watchdog supraviețuiește resetului,
        // dar se șterge la POR. Citit o singură dată și invalidat imediat.
        if (watchdog_hw->scratch[0] == Config::Hardware::RESET_MAGIC_BUTTON) {
            watchdog_hw->scratch[0] = 0;
            // Categoria 4: resetul local declanșat de 4 apăsări rapide.
            return 4;   // Reset declanșat de 4 apăsări rapide buton apel
        }

        // Categoria 2: reset real watchdog, prin timer sau comandă force.
        if (watchdogReason & (WATCHDOG_REASON_TIMER_BITS | WATCHDOG_REASON_FORCE_BITS)) return 2;

        // Categoria 1: registrul grupează POR și brownout în același indicator.
        if (chipResetReason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS) return 1;

        // Categoria 3: RUN, debug, reset software sau cauză nedeterminată.
        return 3;
    }

    void init() {
        // Primul: deschide sectorul EEPROM (256 B) și încarcă setările persistente
        Settings::init();

        Presentation::init();
        Protocol::init();
        LiftController::init();
        Diagnostics::init();

        // NOU: Inițializare Service Mode
        ServiceProtocol::init();
        ServiceMenu::init();

        // EEPROM este deja deschis (256 B) de Settings::init()
        const uint8_t resetReason = getHardwareResetReason();
        const uint16_t curentBoots = SystemCounters::recordBoot(resetReason);

        SharedPanel initPanel = {};
        initPanel.system.bootCounter = curentBoots;
        initPanel.system.lastResetReason = resetReason;
        initPanel.ui.sound.event = SoundEvent::Boot;
        initPanel.ui.sound.seq = 1;

        shared_panel_write(gSharedMemory, initPanel);
    }

    // ==================== CORE 0: Randare + Service Mode ====================
    void runCore0() {
        // Poll comanda seriala de la Service Box (master).
        // TREBUIE sa ruleze pe core0: TinyUSB device (pico_stdio_usb) are
        // IRQ/task pe core0, iar Serial.available/read/println din core1
        // corupeau starea stack-ului (hang aleator la write/flush/delay in
        // secventa BOOTSEL). Tot accesul USB este acum single-core.
        ServiceProtocol::poll();

        const bool wasInServiceMode = ServiceMenu::isInServiceMode();
        ServiceMenu::update();
        const bool returnedFromService = wasInServiceMode && !ServiceMenu::isInServiceMode();

        // Schimbare CabOrder (mb_15): invalidează ambele ecrane ca noul
        // aranjament să fie redesenat imediat, inclusiv în Service Mode.
        static bool sLastCabSwapped = Settings::cabOrderSwapped();
        const bool cabSwapped = Settings::cabOrderSwapped();
        if (cabSwapped != sLastCabSwapped) {
            sLastCabSwapped = cabSwapped;
            PanelRenderer::invalidate(DisplayTarget::Left);
            PanelRenderer::invalidate(DisplayTarget::Right);
        }

        if (returnedFromService) {
            PanelRenderer::invalidate(DisplayTarget::Left);
            PanelRenderer::invalidate(DisplayTarget::Right);
        }

        // Dacă suntem în Service Mode, ServiceMenu preia controlul complet
        if (ServiceMenu::isInServiceMode()) {
            return;
        }

        // Mod Normal: logica existentă neschimbată
        SharedPanel localSnapshot = {};
        static unsigned long lastDiagMillis = 0;

        if (shared_panel_read(gSharedMemory, localSnapshot)) {
            const bool displayWoke = Presentation::update(localSnapshot);

            if (displayWoke) {
                PanelRenderer::invalidate(DisplayTarget::Left);
                PanelRenderer::invalidate(DisplayTarget::Right);
            }

            const DiagnosticsNavigator::NavigatorState &nav = DiagnosticsNavigator::getState();

            // --- ECRAN PRINCIPAL ---
            // CabOrder (mb_15): false = direct (cabina1 pe LCD1/tft1), true = invers.
            // Meniul de diagnostic rămâne fix pe DisplayTarget::Left (LCD1, SPI0).
            const DisplayTarget cab1Target = cabSwapped ? DisplayTarget::Right : DisplayTarget::Left;
            const DisplayTarget cab2Target = cabSwapped ? DisplayTarget::Left : DisplayTarget::Right;
            const DisplayTarget menuTarget = DisplayTarget::Left;

            if (!nav.isMenuOpen || cab2Target != menuTarget) {
                PanelRenderer::render(cab2Target, localSnapshot.lift2, "ASCENSOR 2");
            } else {
                PanelRenderer::invalidate(cab2Target);
            }

            if (!nav.isMenuOpen || cab1Target != menuTarget) {
                PanelRenderer::render(cab1Target, localSnapshot.lift1, "ASCENSOR 1");
            } else {
                PanelRenderer::invalidate(cab1Target);
            }

            // --- MENIU DIAGNOSTIC ---
            if (nav.isMenuOpen) {
                if (millis() - lastDiagMillis >= 500) {
                    lastDiagMillis = millis();
                    ProcessedDiagnostics info;
                    Diagnostics::interpret(localSnapshot, info);
                    PageModel logicalPage;
                    uint8_t pageIdx = (nav.currentProfile == DiagnosticsProfile::Service)
                        ? nav.servicePageIndex : nav.developerPageIndex;
                    UIPresenter::buildPage(info, nav.currentProfile, pageIdx, logicalPage);
                    DisplayRenderer::render(DisplayTarget::Left, logicalPage);
                }
            }

            if (displayWoke) {
                Display::showBacklight();
            }
        }
    }

    // ==================== CORE 1: Comunicație + Service Protocol ====================
    void runCore1() {
        static SharedPanel localPanelCore1 = {};
        static unsigned long lastUptimeUpdate = 0;

        if (!shared_panel_read(gSharedMemory, localPanelCore1)) {
            return;
        }

        Protocol::update(localPanelCore1);

        // Scanăm la fiecare iterație microsecundică butoanele GP16/GP26 (V10.26)
        DiagnosticsNavigator::updateButtons();

        ControllerResult result = LiftController::process(localPanelCore1);
        if (result.transmit) {
            Protocol::trimiteApel(result.lift);
        }

        if (millis() - lastUptimeUpdate >= 1000) {
            lastUptimeUpdate += 1000;
            localPanelCore1.system.uptimeSeconds++;
        }

        shared_panel_write(gSharedMemory, localPanelCore1);
    }
}