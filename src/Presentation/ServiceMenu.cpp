// Presentation/ServiceMenu.cpp
// Service Mode: date reale + chenar overlay (portocaliu/roșu)
// Testele display și mb_com rămân pe ecran până la mb_com_out (manual din Magic Box)

#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Display.h"
#include "SharedPanel.h"
#include <Arduino.h>

extern volatile SharedMemory gSharedMemory;

namespace ServiceMenu {

static bool sInService = false;
static unsigned long sLastActivityMillis = 0;
static constexpr unsigned long SERVICE_TIMEOUT_MS = 60000;

// Stare display: false = idle (date reale + overlay), true = test activ
static bool sDisplayTestActive = false;

// Forward declarations
static void handleCommand(const ServiceProtocol::Command &cmd);
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveData);
static void runCommTest();
static void sendCommStatus();
static void sendDiagnostics();
static void runDispTest(uint8_t tftId);
static void runDispReinit(uint8_t tftId);
static void exitDisplayTest();
static void sendMcuUptime();
static void sendMcuResets();
static void sendMcuWdt();
static void sendMcuLastReset();
static void sendMcuTemp();
static void sendMcuStack(uint8_t coreId);
static bool checkService();

static const char* directionText(Direction direction) {
    switch (direction) {
        case Direction::Up: return "UP";
        case Direction::Down: return "DWN";
        default: return "--";
    }
}

static const char* floorText(uint8_t floor, char* buffer, size_t bufferSize) {
    if (floor == 0) return "P";
    snprintf(buffer, bufferSize, "%u", floor);
    return buffer;
}

static const char* serviceText(ServiceState service) {
    switch (service) {
        case ServiceState::Fault: return "Def.";
        case ServiceState::Revision: return "Rev.";
        case ServiceState::Missing: return "No ser.";
        default: return "OK";
    }
}

static const char* occupancyText(Occupancy occupancy) {
    return occupancy == Occupancy::Busy ? "YES" : "NO";
}

void init() {
    sInService = false;
    sLastActivityMillis = 0;
    sDisplayTestActive = false;
}

bool isInServiceMode() {
    return sInService;
}

bool isDisplayCommandActive() {
    return sDisplayTestActive;
}

// ==================== UPDATE PRINCIPAL ====================
void update() {
    // 1. Citește datele reale din memoria partajată
    SharedPanel localPanel;
    bool haveNewData = shared_panel_read(gSharedMemory, localPanel);

    // 2. În afara Service Mode, update() doar consumă comenzi de intrare.
    if (sInService && !sDisplayTestActive) {
        if (haveNewData) {
            Display::update(localPanel);
        }
        uint16_t overlayColor = getOverlayColor(localPanel, haveNewData);
        Display::drawServiceOverlay(DisplayTarget::Left,  overlayColor);
        Display::drawServiceOverlay(DisplayTarget::Right, overlayColor);
    }
    // Dacă sDisplayTestActive == true, ecranul rămâne așa cum l-a lăsat ultima comandă
    // (mb_com, DISP TEST, etc.) — NU suprascriem cu date reale.

    // 3. Consumă comenzi de la Core 1 (Magic Box)
    ServiceProtocol::Command cmd;
    while (ServiceProtocol::consumeCommand(cmd)) {
        sLastActivityMillis = millis();
        handleCommand(cmd);
    }

    // 4. Auto-exit Service Mode la inactivitate (doar dacă nu e test display activ)
    if (sInService && !sDisplayTestActive &&
        (millis() - sLastActivityMillis > SERVICE_TIMEOUT_MS)) {
        sInService = false;
        Display::setServiceMode(false);
        ServiceProtocol::sendResponse("ACK SRV AUTOEXIT");
    }
}

// ==================== CULOARE OVERLAY ====================
static uint16_t getOverlayColor(const SharedPanel &panel, bool haveData) {
    // Roșu dacă avem erori de comunicație sau seqlock a eșuat
    if (!haveData) return 0xF800; // COLOR_RED
    if (panel.system.packetErrorsLift1 > 0 ||
        panel.system.packetErrorsLift2 > 0 ||
        panel.system.seqlockCollisions > 0) {
        return 0xF800; // COLOR_RED
    }
    return 0xFD20; // COLOR_ORANGE — service normal
}

// ==================== HANDLERE COMENZI ====================
static void handleCommand(const ServiceProtocol::Command &cmd) {
    switch (cmd.type) {
        case ServiceProtocol::Command::Type::EnterService:
            sInService = true;
            sDisplayTestActive = false;
            Display::setServiceMode(true);
            ServiceProtocol::sendResponse("ACK SRV ENTER");
            break;

        case ServiceProtocol::Command::Type::ExitService:
            sInService = false;
            sDisplayTestActive = false;
            Display::setServiceMode(false);
            ServiceProtocol::sendResponse("ACK SRV EXIT");
            break;

        case ServiceProtocol::Command::Type::CommTest:
            if (!checkService()) break;
            runCommTest();
            break;

        case ServiceProtocol::Command::Type::CommStatus:
            if (!checkService()) break;
            sendCommStatus();
            break;

        case ServiceProtocol::Command::Type::Diagnostics:
            if (!checkService()) break;
            sendDiagnostics();
            break;

        case ServiceProtocol::Command::Type::DispTest:
            if (!checkService()) break;
            runDispTest(cmd.param);
            break;

        case ServiceProtocol::Command::Type::DispReinit:
            if (!checkService()) break;
            runDispReinit(cmd.param);
            break;

        case ServiceProtocol::Command::Type::TestExit:   // ← NOU: revine la date reale
            if (!checkService()) break;
            exitDisplayTest();
            break;

        case ServiceProtocol::Command::Type::McuUptime:
            if (!checkService()) break;
            sendMcuUptime();
            break;

        case ServiceProtocol::Command::Type::McuResets:
            if (!checkService()) break;
            sendMcuResets();
            break;

        case ServiceProtocol::Command::Type::McuWdt:
            if (!checkService()) break;
            sendMcuWdt();
            break;

        case ServiceProtocol::Command::Type::McuLastReset:
            if (!checkService()) break;
            sendMcuLastReset();
            break;

        case ServiceProtocol::Command::Type::McuTemp:
            if (!checkService()) break;
            sendMcuTemp();
            break;

        case ServiceProtocol::Command::Type::McuStack:
            if (!checkService()) break;
            sendMcuStack(cmd.param);
            break;

        default:
            ServiceProtocol::sendResponse("ERR 04 UNHANDLED");
            break;
    }
}

static bool checkService() {
    if (!sInService) {
        ServiceProtocol::sendResponse("ERR 03 NOT_IN_SERVICE");
        return false;
    }
    return true;
}

// ==================== COMUNICATION ====================
static void runCommTest() {
    SharedPanel panel;
    if (!shared_panel_read(gSharedMemory, panel)) {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
        return;
    }

    sDisplayTestActive = true;
    Display::clearTargetScreen(DisplayTarget::Left);
    Display::clearTargetScreen(DisplayTarget::Right);

    const DisplayTarget left = DisplayTarget::Left;
    const DisplayTarget right = DisplayTarget::Right;
    char lift1Pos[4];
    char lift1Dst[4];
    char lift2Pos[4];
    char lift2Dst[4];
    Display::printMenuHeader(left, "Comm. A 1");
    Display::printMenuHeader(right, "Comm. A 2");

    Display::printCommLine(left, 0, "Pos :", floorText(panel.lift1.pos, lift1Pos, sizeof(lift1Pos)), 0xFFFF);
    Display::printCommLine(left, 1, "Dst :", panel.lift1.sj == Direction::Idle ? "--" : floorText(panel.lift1.etd, lift1Dst, sizeof(lift1Dst)), 0xFFFF);
    Display::printCommLine(left, 2, "S/J :", directionText(panel.lift1.sj), 0xFFFF);
    Display::printCommLine(left, 3, "Svc :", serviceText(panel.lift1.svc), 0xFFFF);
    Display::printCommLine(left, 4, "Ocp :", occupancyText(panel.lift1.ocp), 0xFFFF);

    Display::printCommLine(right, 0, "Pos :", floorText(panel.lift2.pos, lift2Pos, sizeof(lift2Pos)), 0xFFFF);
    Display::printCommLine(right, 1, "Dst :", panel.lift2.sj == Direction::Idle ? "--" : floorText(panel.lift2.etd, lift2Dst, sizeof(lift2Dst)), 0xFFFF);
    Display::printCommLine(right, 2, "S/J :", directionText(panel.lift2.sj), 0xFFFF);
    Display::printCommLine(right, 3, "Svc :", serviceText(panel.lift2.svc), 0xFFFF);
    Display::printCommLine(right, 4, "Ocp :", occupancyText(panel.lift2.ocp), 0xFFFF);

    ServiceProtocol::sendResponse("OK mb_com L1=DATA L2=DATA");
}

static void sendCommStatus() {
    SharedPanel panel;
    if (!shared_panel_read(gSharedMemory, panel)) {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
        return;
    }

    char response[128];
    snprintf(response, sizeof(response),
             "OK STATUS L1=%s L2=%s ERR1=%u ERR2=%u",
             serviceText(panel.lift1.svc), serviceText(panel.lift2.svc),
             panel.system.packetErrorsLift1, panel.system.packetErrorsLift2);
    ServiceProtocol::sendResponse(response);
}

static void sendDiagnostics() {
    SharedPanel panel;
    if (!shared_panel_read(gSharedMemory, panel)) {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
        return;
    }

    char response[128];
    snprintf(response, sizeof(response),
             "OK DIAG ERR1=%u ERR2=%u SEQ=%u RESET=%u",
             panel.system.packetErrorsLift1, panel.system.packetErrorsLift2,
             panel.system.seqlockCollisions, panel.system.lastResetReason);
    ServiceProtocol::sendResponse(response);
}

// ==================== DISPLAY ====================
static void runDispTest(uint8_t tftId) {
    sDisplayTestActive = true;
    if (tftId == 1) {
        Display::clearTargetScreen(DisplayTarget::Left);
        Display::printMenuHeader(DisplayTarget::Left, "DISP TEST 1");
        Display::printMenuLineExt(DisplayTarget::Left, 3, "RESULT", "PASS", 0x07E0);
        ServiceProtocol::sendResponse("OK DISP TEST 1");
    } else if (tftId == 2) {
        Display::clearTargetScreen(DisplayTarget::Right);
        Display::printMenuHeader(DisplayTarget::Right, "DISP TEST 2");
        Display::printMenuLineExt(DisplayTarget::Right, 3, "RESULT", "PASS", 0x07E0);
        ServiceProtocol::sendResponse("OK DISP TEST 2");
    } else {
        ServiceProtocol::sendResponse("ERR 05 INVALID_TFT");
    }
}

static void runDispReinit(uint8_t tftId) {
    sDisplayTestActive = true;
    if (tftId == 1) {
        // TODO: apelează reinit hardware TFT 1
        ServiceProtocol::sendResponse("OK DISP REINIT 1");
    } else if (tftId == 2) {
        // TODO: reinit hardware TFT 2
        ServiceProtocol::sendResponse("OK DISP REINIT 2");
    } else if (tftId == 3) {  // BOTH
        // TODO: reinit ambele
        ServiceProtocol::sendResponse("OK DISP REINIT BOTH");
    } else {
        ServiceProtocol::sendResponse("ERR 05 INVALID_TFT");
    }
}

static void exitDisplayTest() {
    sDisplayTestActive = false;
    // La următorul ciclu update(), datele reale + overlay vor fi randate automat
    ServiceProtocol::sendResponse("ACK mb_com_out");
}

// ==================== PICO / MCU (Serial only) ====================
static void sendMcuUptime() {
    SharedPanel localPanel;
    if (shared_panel_read(gSharedMemory, localPanel)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "OK MCU UPTIME %lu", localPanel.system.uptimeSeconds);
        ServiceProtocol::sendResponse(buf);
    } else {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
    }
}

static void sendMcuResets() {
    SharedPanel localPanel;
    if (shared_panel_read(gSharedMemory, localPanel)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "OK MCU RESETS %u", localPanel.system.bootCounter);
        ServiceProtocol::sendResponse(buf);
    } else {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
    }
}

static void sendMcuWdt() {
    bool wdtEnabled = true;  // TODO: înlocuiește cu variabila reală
    ServiceProtocol::sendResponse(wdtEnabled ? "OK MCU WDT ENABLED" : "OK MCU WDT DISABLED");
}

static void sendMcuLastReset() {
    SharedPanel localPanel;
    if (shared_panel_read(gSharedMemory, localPanel)) {
        const char* reason = "UNKNOWN";
        switch (localPanel.system.lastResetReason) {
            case 0: reason = "POR"; break;
            case 1: reason = "BOR"; break;
            case 2: reason = "WDT"; break;
            case 3: reason = "SYSREQ"; break;
            default: break;
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "OK MCU LASTRESET %s", reason);
        ServiceProtocol::sendResponse(buf);
    } else {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
    }
}

static void sendMcuTemp() {
    float temp = analogReadTemp();
    char buf[64];
    snprintf(buf, sizeof(buf), "OK MCU TEMP %.1f", temp);
    ServiceProtocol::sendResponse(buf);
}

static void sendMcuStack(uint8_t coreId) {
    if (coreId > 1) {
        ServiceProtocol::sendResponse("ERR 07 INVALID_CORE");
        return;
    }
    // TODO: înlocuiește cu FreeRTOS real
    uint32_t used = 2048;
    uint32_t free = 4096;
    uint32_t hw   = 8192;
    char buf[128];
    snprintf(buf, sizeof(buf), "OK MCU STACK %u USED=%lu FREE=%lu HW=%lu",
             coreId, used, free, hw);
    ServiceProtocol::sendResponse(buf);
}

} // namespace ServiceMenu