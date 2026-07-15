//Logic/Application.cpp (Orchestratorul General cu Persistență Flash)

#include "Arduino.h"
#include "Application.h" 

// 1. Rute către folderul src/ (Urcat un nivel)
#include "SharedPanel.h"
#include "BuildInfo.h"
#include "Pins.h"        // <--- REPARAT: Oferă vizibilitatea namespace-ului Pins

// 2. Rute interne în cadrul aceluiași folder src/Logic/
#include "Presentation.h"
#include "Protocol.h"
#include "LiftController.h"

// 3. Rute către folderul src/Drivers/ (Urcat un nivel și intrat în Drivers)
#include "../Drivers/Display.h"   // <--- REPARAT: Oferă vizibilitatea pentru DisplayTarget

// 4. Rute către tab-urile rămase în rădăcina mare a proiectului (Urcat două niveluri)
#include "Diagnostics.h"       
#include "DiagnosticsNavigator.h" 
#include "UIPresenter.h"      // <--- REPARAT: Oferă vizibilitatea pentru UIPresenter
#include "DisplayRenderer.h"  // <--- REPARAT: Oferă vizibilitatea pentru DisplayRenderer

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

        
    void runCore0() {
        SharedPanel localSnapshot = {};
        static unsigned long lastDiagMillis = 0;

        if (shared_panel_read(gSharedMemory, localSnapshot)) {
            Presentation::update(localSnapshot);

            // Citim starea asincronă a butoanelor din spatele panoului procesată de Core 1
            const DiagnosticsNavigator::NavigatorState &nav = DiagnosticsNavigator::getState();
            
            if (nav.isMenuOpen) {
                // Rulăm randarea o dată la 500ms pentru fluiditate optică, fără a bloca bus-ul SPI
                if (millis() - lastDiagMillis >= 500) {
                    lastDiagMillis = millis();
                    
                    ProcessedDiagnostics info;
                    Diagnostics::interpret(localSnapshot, info);
                    
                    // 1. Instanțiem modelul logic pur în memoria stivei Core 0
                    PageModel logicalPage;
                    
                    // 2. Deducem indexul corect pe baza profilului selectat nativ de navigator
                    uint8_t pageIdx = (nav.currentProfile == DiagnosticsProfile::Service) 
                                      ? nav.servicePageIndex 
                                      : nav.developerPageIndex;
                    
                    // 3. UIPresenter asamblează modelul abstract în memorie (Fără pixeli)
                    UIPresenter::buildPage(info, nav.currentProfile, pageIdx, logicalPage);
                    
                    // 4. Adaptorul desenează mecanic modelul pe ecranul din Stânga
                    DisplayRenderer::render(DisplayTarget::Left, logicalPage);
                }
            }
        }
    }



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