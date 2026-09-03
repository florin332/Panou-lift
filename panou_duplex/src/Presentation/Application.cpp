// src/Presentation/Application.cpp (Orchestratorul General cu Persistență Flash + Service Mode)

#include "Arduino.h"
#include "Application.h"

// 1. Rute către folderul src/ (Urcat un nivel)
#include "SharedPanel.h"
#include "BuildInfo.h"
#include "Pins.h"

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
#include "ServiceMenu.h"

#include <EEPROM.h>
#include "hardware/structs/vreg_and_chip_reset.h"

namespace Application
{
    constexpr int EEPROM_BOOT_COUNT_ADDR = 0;
    constexpr int EEPROM_SIZE = 4;

    static uint8_t getHardwareResetReason() {
        uint32_t reason = vreg_and_chip_reset_hw->chip_reset;
        if (reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_BITS) return 2;
        if (reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS) return 1;
        return 3;
    }

    void init() {
        Presentation::init();
        Protocol::init();
        LiftController::init();
        Diagnostics::init();

        // NOU: Inițializare Service Mode
        ServiceProtocol::init();
        ServiceMenu::init();

        EEPROM.begin(EEPROM_SIZE);
        uint16_t curentBoots = 0;
        EEPROM.get(EEPROM_BOOT_COUNT_ADDR, curentBoots);
        if (curentBoots == 0xFFFF) { curentBoots = 0; }
        curentBoots++;
        EEPROM.put(EEPROM_BOOT_COUNT_ADDR, curentBoots);
        EEPROM.commit();

        SharedPanel initPanel = {};
        initPanel.system.bootCounter = curentBoots;
        initPanel.system.lastResetReason = getHardwareResetReason();
        initPanel.ui.sound.event = SoundEvent::Boot;
        initPanel.ui.sound.seq = 1;

        shared_panel_write(gSharedMemory, initPanel);
    }

    // ==================== CORE 0: Randare + Service Mode ====================
    void runCore0() {
        const bool wasInServiceMode = ServiceMenu::isInServiceMode();
        ServiceMenu::update();
        const bool returnedFromService = wasInServiceMode && !ServiceMenu::isInServiceMode();

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
            PanelRenderer::render(DisplayTarget::Right, localSnapshot.lift2, "ASCENSOR 2");

            if (!nav.isMenuOpen) {
                PanelRenderer::render(DisplayTarget::Left, localSnapshot.lift1, "ASCENSOR 1");
            } else {
                PanelRenderer::invalidate(DisplayTarget::Left);
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

        // NOU: Poll comandă serial de la Magic Box (master)
        // Rulează pe Core 1 (comunicație) pentru a nu bloca randarea pe Core 0
        ServiceProtocol::poll();
    }
}