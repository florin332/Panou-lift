// Presentation/ServiceMenu.cpp
// Service Mode: date reale + chenar overlay (portocaliu/rosu)

#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Display.h"
#include "SharedPanel.h"
#include "PanelRenderer.h"
#include "BuildInfo.h"
#include "Settings.h"
#include "Protocol.h"
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
static uint8_t sCommunicationDisplayLifts = 0;
static uint32_t sLastCommunicationValues[2][6] = {};
static bool sCommunicationValuesDirty[2] = {true, true};

// Forward declarations
static void handleCommand(const ServiceProtocol::Command &cmd);
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveNewData);
static bool checkService();
static bool isColorTestTarget(DisplayTarget target);
static bool isCommunicationDisplayActiveForTarget(DisplayTarget target);
static DisplayTarget targetForLift(uint8_t lift);
static void drawCommunicationPages(uint8_t liftMask);
static void drawCommunicationPage(DisplayTarget target);
static void renderCommunicationCounters(DisplayTarget target, uint8_t lift,
                                        const CommLineCounters &counters);
static void drawColorTestPage();
static void updateColorTest();
static void stopColorTest();

void init() {
    sInService.store(false, std::memory_order_relaxed);
    sLastActivityMillis = 0;
    sColorTestActive = false;
    sColorTestTargets = 0;
    sCommunicationDisplayLifts = 0;
    Protocol::setPassThroughEnabled(false);
    sCommunicationValuesDirty[0] = true;
    sCommunicationValuesDirty[1] = true;
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
            if (!isCommunicationDisplayActiveForTarget(lift2Target) && !isColorTestTarget(lift2Target)) {
                PanelRenderer::render(lift2Target, localPanel.lift2, "ASCENSOR 2");
            }
            if (!isCommunicationDisplayActiveForTarget(lift1Target) && !isColorTestTarget(lift1Target)) {
                PanelRenderer::render(lift1Target, localPanel.lift1, "ASCENSOR 1");
            }
            if (sCommunicationDisplayLifts & 0x01u) {
                renderCommunicationCounters(targetForLift(1), 1, localPanel.comm.lift1);
            }
            if (sCommunicationDisplayLifts & 0x02u) {
                renderCommunicationCounters(targetForLift(2), 2, localPanel.comm.lift2);
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
            Protocol::setPassThroughEnabled(true);
            Display::setServiceMode(true);
            char panelId[16];
            snprintf(panelId, sizeof(panelId), "D_%02u_%02u.%02u",
                     static_cast<unsigned>(BuildInfo::HardwareVariant),
                     static_cast<unsigned>(BuildInfo::Major),
                     static_cast<unsigned>(BuildInfo::Minor));
            ServiceProtocol::sendResponse(panelId);
            break;

        case ServiceProtocol::Command::Type::EnablePassThrough:
            if (!checkService()) break;
            Protocol::setPassThroughEnabled(true);
            ServiceProtocol::sendResponse("ACK_81");
            break;

        case ServiceProtocol::Command::Type::DisablePassThrough:
            if (!checkService()) break;
            Protocol::setPassThroughEnabled(false);
            ServiceProtocol::sendResponse("ACK_82");
            break;

        case ServiceProtocol::Command::Type::ExitService:
            stopColorTest();
            sCommunicationDisplayLifts = 0;
            Protocol::commCountDisable(1);
            Protocol::commCountDisable(2);
            Protocol::setPassThroughEnabled(false);
            PanelRenderer::invalidate(DisplayTarget::Left);
            PanelRenderer::invalidate(DisplayTarget::Right);
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
            for (uint8_t lift = 1; lift <= 2; lift++) {
                const uint8_t liftMask = static_cast<uint8_t>(1u << (lift - 1));
                const DisplayTarget target = targetForLift(lift);
                if ((sCommunicationDisplayLifts & liftMask) && !isColorTestTarget(target)) {
                    drawCommunicationPage(target);
                }
            }
            char response[16];
            snprintf(response, sizeof(response), "ACK_31 %u", static_cast<unsigned>(cmd.argument));
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
            snprintf(testResponse, sizeof(testResponse), "ACK_32 %u", static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(testResponse);
            break;
        }

        case ServiceProtocol::Command::Type::StopDisplayTest:
            if (!checkService()) break;
            stopColorTest();
            drawCommunicationPages(sCommunicationDisplayLifts);
            ServiceProtocol::sendResponse("ACK_33");
            break;

        case ServiceProtocol::Command::Type::EnterCommunicationDisplay:
            if (!checkService()) break;
            stopColorTest();
            sCommunicationDisplayLifts |= cmd.argument;
            drawCommunicationPages(sCommunicationDisplayLifts);
            char commStartResponse[16];
            snprintf(commStartResponse, sizeof(commStartResponse), "ACK_61 %u",
                     static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(commStartResponse);
            break;

        case ServiceProtocol::Command::Type::ExitCommunicationDisplay:
            if (!checkService()) break;
            sCommunicationDisplayLifts &= static_cast<uint8_t>(~cmd.argument);
            for (uint8_t lift = 1; lift <= 2; lift++) {
                if (cmd.argument & (1u << (lift - 1))) {
                    PanelRenderer::invalidate(targetForLift(lift));
                }
            }
            char commStopResponse[16];
            snprintf(commStopResponse, sizeof(commStopResponse), "ACK_62 %u",
                     static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(commStopResponse);
            break;

        case ServiceProtocol::Command::Type::StartCommunicationCounters:
        case ServiceProtocol::Command::Type::StopCommunicationCounters: {
            if (!checkService()) break;
            const bool start = cmd.type == ServiceProtocol::Command::Type::StartCommunicationCounters;
            for (uint8_t lift = 1; lift <= 2; lift++) {
                if (cmd.argument == 3 || cmd.argument == lift) {
                    if (start) Protocol::commCountEnable(lift);
                    else Protocol::commCountDisable(lift);
                }
            }
            char counterResponse[16];
            snprintf(counterResponse, sizeof(counterResponse), "ACK_%u %u",
                     start ? 63u : 64u, static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(counterResponse);
            break;
        }

        case ServiceProtocol::Command::Type::ResetCommunicationCounters: {
            if (!checkService()) break;
            bool resetQueued = true;
            for (uint8_t lift = 1; lift <= 2; lift++) {
                if ((cmd.argument == 3 || cmd.argument == lift) &&
                    !Protocol::commCountReset(lift)) {
                    resetQueued = false;
                }
            }
            if (!resetQueued) {
                ServiceProtocol::sendResponse("ERR 08 INVALID_PARAM");
                break;
            }
            char resetResponse[16];
            snprintf(resetResponse, sizeof(resetResponse), "ACK_65 %u",
                     static_cast<unsigned>(cmd.argument));
            ServiceProtocol::sendResponse(resetResponse);
            break;
        }

        case ServiceProtocol::Command::Type::QueryCommunicationDisplay: {
            char response[32];
            snprintf(response, sizeof(response), "OK COMM DISPLAY %u",
                     static_cast<unsigned>(sCommunicationDisplayLifts));
            ServiceProtocol::sendResponse(response);
            break;
        }

        case ServiceProtocol::Command::Type::QueryCommunicationCounters: {
            if (!checkService()) break;
            uint8_t enabledMask = 0;
            for (uint8_t lift = 1; lift <= 2; lift++) {
                if (Protocol::commCountIsEnabled(lift)) {
                    enabledMask |= static_cast<uint8_t>(1u << (lift - 1));
                }
            }
            char response[32];
            snprintf(response, sizeof(response), "OK COMM COUNTERS %u",
                     static_cast<unsigned>(enabledMask));
            ServiceProtocol::sendResponse(response);
            break;
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

static bool isColorTestTarget(DisplayTarget target) {
    const uint8_t targetBit = (target == DisplayTarget::Left) ? 1u : 2u;
    return sColorTestActive && (sColorTestTargets & targetBit) != 0;
}

static bool isCommunicationDisplayActiveForTarget(DisplayTarget target) {
    const uint8_t lift = (target == targetForLift(1)) ? 1 : 2;
    return (sCommunicationDisplayLifts & (1u << (lift - 1))) != 0;
}

static DisplayTarget targetForLift(uint8_t lift) {
    const bool swapped = Settings::cabOrderSwapped();
    if (lift == 1) return swapped ? DisplayTarget::Right : DisplayTarget::Left;
    return swapped ? DisplayTarget::Left : DisplayTarget::Right;
}

static void drawCommunicationPages(uint8_t liftMask) {
    for (uint8_t lift = 1; lift <= 2; lift++) {
        if (liftMask & (1u << (lift - 1))) {
            const DisplayTarget target = targetForLift(lift);
            PanelRenderer::invalidate(target);
            drawCommunicationPage(target);
        }
    }
}

static void drawCommunicationPage(DisplayTarget target) {
    Display::clearTargetScreen(target);
    const uint8_t lift = (target == targetForLift(1)) ? 1 : 2;
    static const char *labels[] = {"Total", "Valid", "Timeout", "Format", "CRC", "Data"};
    static const uint16_t labelColors[] = {0xFFFF, 0xFFFF, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0};
    for (uint8_t row = 0; row < 6; row++) {
        Display::printCommLabel(target, row, labels[row], labelColors[row]);
        Display::printCommValue(target, row, 0, 0, 0x07E0);
        sLastCommunicationValues[lift - 1][row] = 0;
    }
    sCommunicationValuesDirty[lift - 1] = true;
}

static void renderCommunicationCounters(DisplayTarget target, uint8_t lift,
                                        const CommLineCounters &counters) {
    const uint32_t values[] = {
        counters.rxFrames,
        counters.rxValid,
        counters.rxTimeout,
        counters.rxFormatError,
        counters.rxCrcError,
        counters.rxDataError
    };
    const uint8_t index = lift - 1;

    for (uint8_t row = 0; row < 6; row++) {
        if (sCommunicationValuesDirty[index] || sLastCommunicationValues[index][row] != values[row]) {
            Display::printCommValue(target, row, sLastCommunicationValues[index][row], values[row], 0x07E0);
            sLastCommunicationValues[index][row] = values[row];
        }
    }
    sCommunicationValuesDirty[index] = false;
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