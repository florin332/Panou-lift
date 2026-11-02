// Presentation/ServiceMenu.cpp
// Service Mode: date reale + chenar overlay (portocaliu/rosu)

#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Display.h"
#include "SharedPanel.h"
#include "PanelRenderer.h"
#include "BuildInfo.h"
#include "Settings.h"
#include <Arduino.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <atomic>

extern volatile SharedMemory gSharedMemory;

namespace ServiceMenu {

static std::atomic<bool> sInService{false};
static unsigned long sLastActivityMillis = 0;
static bool sColorTestActive = false;
static uint8_t sColorTestTargets = 0;
static uint8_t sColorTestColorIndex = 0;
static bool sColorTestLabelAtTop = true;
static unsigned long sColorTestLastStepMillis = 0;

// Forward declarations
static void handleCommand(const ServiceProtocol::Command &cmd);
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveNewData);
static bool checkService();
static bool isColorTestTarget(DisplayTarget target);
static void drawColorTestPage();
static void updateColorTest();
static void stopColorTest();

void init() {
    sInService.store(false, std::memory_order_relaxed);
    sLastActivityMillis = 0;
    sColorTestActive = false;
    sColorTestTargets = 0;
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
            // CabOrder (mb_15): la swap, cabina1 se randează pe LCD2 și invers
            const bool swapped = Settings::cabOrderSwapped();
            const DisplayTarget lift2Target = swapped ? DisplayTarget::Left : DisplayTarget::Right;
            const DisplayTarget lift1Target = swapped ? DisplayTarget::Right : DisplayTarget::Left;
            if (!isColorTestTarget(lift2Target)) {
                PanelRenderer::render(lift2Target, localPanel.lift2, "ASCENSOR 2");
            }
            if (!isColorTestTarget(lift1Target)) {
                PanelRenderer::render(lift1Target, localPanel.lift1, "ASCENSOR 1");
            }
        }
        updateColorTest();
        uint16_t overlayColor = getOverlayColor(localPanel, haveNewData);
        if (!isColorTestTarget(DisplayTarget::Left)) {
            Display::drawServiceOverlay(DisplayTarget::Left, overlayColor);
        }
        if (!isColorTestTarget(DisplayTarget::Right)) {
            Display::drawServiceOverlay(DisplayTarget::Right, overlayColor);
        }
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
            stopColorTest();
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

        case ServiceProtocol::Command::Type::ResetDisplay:
            if (!checkService()) break;
            if (cmd.argument == 1 || cmd.argument == 3) {
                Display::reinitializeTarget(DisplayTarget::Left);
                PanelRenderer::invalidate(DisplayTarget::Left);
            }
            if (cmd.argument == 2 || cmd.argument == 3) {
                Display::reinitializeTarget(DisplayTarget::Right);
                PanelRenderer::invalidate(DisplayTarget::Right);
            }
            char response[16];
            snprintf(response, sizeof(response), "ACK_16 %u", static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(response);
            break;

        case ServiceProtocol::Command::Type::StartDisplayTest: {
            if (!checkService()) break;
            stopColorTest();
            sColorTestTargets = cmd.argument;
            sColorTestColorIndex = 0;
            sColorTestLabelAtTop = true;
            sColorTestLastStepMillis = millis();
            sColorTestActive = true;
            drawColorTestPage();
            char testResponse[16];
            snprintf(testResponse, sizeof(testResponse), "ACK_17 %u", static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(testResponse);
            break;
        }

        case ServiceProtocol::Command::Type::StopDisplayTest:
            if (!checkService()) break;
            stopColorTest();
            ServiceProtocol::sendResponse("ACK_18");
            break;

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

static bool isColorTestTarget(DisplayTarget target) {
    const uint8_t targetBit = (target == DisplayTarget::Left) ? 1u : 2u;
    return sColorTestActive && (sColorTestTargets & targetBit) != 0;
}

static void drawColorTestPage() {
    if (sColorTestTargets & 1u) {
        Display::drawColorTestPage(DisplayTarget::Left, sColorTestColorIndex, sColorTestLabelAtTop);
    }
    if (sColorTestTargets & 2u) {
        Display::drawColorTestPage(DisplayTarget::Right, sColorTestColorIndex, sColorTestLabelAtTop);
    }
}

static void updateColorTest() {
    if (!sColorTestActive || millis() - sColorTestLastStepMillis < 3000) return;

    sColorTestLastStepMillis = millis();
    sColorTestColorIndex++;
    if (sColorTestColorIndex >= 5) {
        sColorTestColorIndex = 0;
        sColorTestLabelAtTop = !sColorTestLabelAtTop;
    }
    drawColorTestPage();
}

static void stopColorTest() {
    if (!sColorTestActive) return;
    if (sColorTestTargets & 1u) PanelRenderer::invalidate(DisplayTarget::Left);
    if (sColorTestTargets & 2u) PanelRenderer::invalidate(DisplayTarget::Right);
    sColorTestActive = false;
    sColorTestTargets = 0;
}

} // namespace ServiceMenu