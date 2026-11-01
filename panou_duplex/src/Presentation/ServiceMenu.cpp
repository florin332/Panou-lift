// Presentation/ServiceMenu.cpp
// Service Mode: date reale + chenar overlay (portocaliu/rosu)

#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Display.h"
#include "SharedPanel.h"
#include "PanelRenderer.h"
#include "BuildInfo.h"
#include <Arduino.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <atomic>

extern volatile SharedMemory gSharedMemory;

namespace ServiceMenu {

static std::atomic<bool> sInService{false};
static unsigned long sLastActivityMillis = 0;

// Forward declarations
static void handleCommand(const ServiceProtocol::Command &cmd);
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveNewData);
static bool checkService();

void init() {
    sInService.store(false, std::memory_order_relaxed);
    sLastActivityMillis = 0;
}

bool isInServiceMode() {
    return sInService.load(std::memory_order_relaxed);
}

// ==================== UPDATE PRINCIPAL ====================
void update() {
    // 1. Citeste datele reale din memoria partajata
    SharedPanel localPanel;
    bool haveNewData = shared_panel_read(gSharedMemory, localPanel);

    // 2. In afara Service Mode, update() doar consuma comenzi de intrare.
    if (sInService.load(std::memory_order_relaxed)) {
        if (haveNewData) {
            Display::update(localPanel);
            PanelRenderer::render(DisplayTarget::Right, localPanel.lift2, "ASCENSOR 2");
            PanelRenderer::render(DisplayTarget::Left, localPanel.lift1, "ASCENSOR 1");
        }
        uint16_t overlayColor = getOverlayColor(localPanel, haveNewData);
        Display::drawServiceOverlay(DisplayTarget::Left, overlayColor);
        Display::drawServiceOverlay(DisplayTarget::Right, overlayColor);
    }

    // 3. Consuma comenzi de la Core 1 (Magic Box)
    ServiceProtocol::Command cmd;
    while (ServiceProtocol::consumeCommand(cmd)) {
        sLastActivityMillis = millis();
        handleCommand(cmd);
    }
}

// ==================== CULOARE OVERLAY ====================
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveNewData) {
    if (!haveNewData) return 0xF800; // COLOR_RED
    if (panel.system.seqlockCollisions > 0) {
        return 0xF800; // COLOR_RED
    }
    return 0xFD20; // COLOR_ORANGE
}

// ==================== HANDLERE COMENZI ====================
static void handleCommand(const ServiceProtocol::Command &cmd) {
    switch (cmd.type) {
        case ServiceProtocol::Command::Type::EnterService:
            sInService.store(true, std::memory_order_relaxed);
            Display::setServiceMode(true);
            char panelId[16];
            snprintf(panelId, sizeof(panelId), "D_%02u_%02u.%02u",
                     static_cast<unsigned>(BuildInfo::HardwareVariant),
                     static_cast<unsigned>(BuildInfo::Major),
                     static_cast<unsigned>(BuildInfo::Minor));
            ServiceProtocol::sendResponse(panelId);
            break;

        case ServiceProtocol::Command::Type::ExitService:
            sInService.store(false, std::memory_order_relaxed);
            Display::setServiceMode(false);
            ServiceProtocol::sendResponse("ACK_00");
            break;

        case ServiceProtocol::Command::Type::EnterBootsel:
            if (!checkService()) break;
            // Suntem pe core0 (acelasi core cu task-ul TinyUSB).
            // FARA Serial.flush(): atarna si de pe core0 (deadlock pe
            // mutex-ul TinyUSB, inca atins de RX-ul Serial.available/read
            // din ServiceProtocol::poll() de pe core1). Scrierile simple
            // s-au dovedit a iesi pe fir fara flush; delay-ul acopera
            // golirea FIFO-ului inainte de reset.
            ServiceProtocol::sendResponse("ACK_02");
            ServiceProtocol::sendResponse("DBG S1 ack-trimis");
            delay(150);       // lasa hostul (Service Box) sa consume FIFO-ul
            ServiceProtocol::sendResponse("DBG S3 pre-reset");
            delay(20);
            // ROM-ul BOOTSEL ruleaza in timp ce core1 inca executa Protocol
            // din flash. Oprirea explicita a core1 elimina accesul concurent
            // la XIP/periferice pe durata handoff-ului spre bootrom.
            multicore_reset_core1();
            // watchdog_reboot (apelat din reset_usb_boot) atarna la infinit
            // daca tick-ul watchdog-ului nu ruleaza; pornim explicit tick-ul
            // (clk_ref = XOSC 12 MHz / 12 = 1 MHz) inainte de apelul ROM.
            watchdog_start_tick(12);
            reset_usb_boot(0, 0);
            // Nu ar trebui sa se ajunga aici; pastram markerul pentru cazul
            // anormal in care ROM-ul ar returna.
            while (true) {
                ServiceProtocol::sendResponse("DBG S4 rom-a-returnat");
                delay(500);
            }

        default:
            ServiceProtocol::sendResponse("ERR 04 UNHANDLED");
            break;
    }
}

static bool checkService() {
    if (!sInService.load(std::memory_order_relaxed)) {
        ServiceProtocol::sendResponse("ERR 03 NOT_IN_SERVICE");
        return false;
    }
    return true;
}

} // namespace ServiceMenu