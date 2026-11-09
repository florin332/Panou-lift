// Presentation/ServiceMenu.cpp
// Service Mode: date reale + chenar overlay (portocaliu/rosu)
// Pagina COMM ramane pe ecran pana la mb_66 0 (manual din Service Box)

#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Graphics.h"
#include "DisplayGeometry.h"
#include "BuildInfo.h"
#include "SharedPanel.h"
#include "PanelRenderer.h"
#include "ElevatorService.h"
#include "SDCard.h"
#include <Arduino.h>
#include <Fonts/FreeSans12pt7b.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include <atomic>
#include <cmath>

namespace ServiceMenu {

static std::atomic<bool> sInService{false};
static unsigned long sLastActivityMillis = 0;

enum class ServicePageMode : uint8_t { Live, Communication, RxCounters, ColorTest };
static ServicePageMode sServicePageMode = ServicePageMode::Live;
static uint8_t sColorTestIndex = 0;
static bool sColorTestLabelAtTop = true;
static unsigned long sColorTestLastStepMillis = 0;
static constexpr unsigned long COLOR_TEST_STEP_MS = 3000;
static uint32_t sLastRxCounterValues[6] = {};
static bool sRxCounterValuesDirty = true;

// Forward declarations
static void handleCommand(const ServiceProtocol::Command &cmd);
static uint16_t getOverlayColor(bool haveNewData);
static void restoreServiceMain();
static void closeCommPage();
static void showCommPage();
static void drawCommPage(const SharedPanel &panel);
static void drawRxCounterPage(const SharedPanel &panel);
static void renderRxCounterValues(const SharedPanel &panel);
static void startColorTest(uint8_t target);
static void stopColorTest();
static void updateColorTest();
static void sendMcuUptime();
static void sendMcuLastReset();
static void sendMcuTemp();
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
    sInService.store(false, std::memory_order_relaxed);
    sLastActivityMillis = 0;
    sServicePageMode = ServicePageMode::Live;
    sColorTestIndex = 0;
    sColorTestLabelAtTop = true;
    sColorTestLastStepMillis = 0;
    sRxCounterValuesDirty = true;
}

bool isInServiceMode() {
    return sInService.load(std::memory_order_relaxed);
}

void enterBootsel() {
    if (get_core_num() != 0) {
        ServiceProtocol::sendResponse("ERR 06 INVALID_CONTEXT");
        return;
    }
    if (!checkService()) return;

    ServiceProtocol::sendResponse("ACK_02");
    ServiceProtocol::sendResponse("DBG S1 ack-trimis");
    delay(150);
    ServiceProtocol::sendResponse("DBG S3 pre-reset");
    delay(20);
    multicore_reset_core1();
    watchdog_start_tick(12);
    reset_usb_boot(0, 0);
    while (true) {
        ServiceProtocol::sendResponse("DBG S4 rom-a-returnat");
        delay(500);
    }
}

// ==================== UPDATE PRINCIPAL ====================
void update() {
    // 1. Citeste datele reale din memoria partajata
    SharedPanel localPanel;
    bool haveNewData = ApplicationState::read(localPanel);

    // 2. In afara Service Mode, update() doar consuma comenzi de intrare.
    if (sInService.load(std::memory_order_relaxed) && sServicePageMode == ServicePageMode::Live) {
        if (haveNewData) {
            Graphics::update(localPanel);
            PanelRenderer::render(GraphicsTarget::Panel, localPanel.lift1, "ASCENSOR");
        }
        uint16_t overlayColor = getOverlayColor(haveNewData);
        Graphics::drawServiceOverlay(GraphicsTarget::Panel, overlayColor);
    }
    if (sInService.load(std::memory_order_relaxed) && sServicePageMode == ServicePageMode::ColorTest) {
        updateColorTest();
    }
    if (sInService.load(std::memory_order_relaxed) && sServicePageMode == ServicePageMode::RxCounters) {
        if (haveNewData) renderRxCounterValues(localPanel);
        Graphics::drawServiceOverlay(GraphicsTarget::Panel, getOverlayColor(haveNewData));
    }

    // 3. Consuma comenzile service trimise de Core 0.
    ServiceProtocol::Command cmd;
    while (ServiceProtocol::consumeCommand(cmd)) {
        sLastActivityMillis = millis();
        handleCommand(cmd);
    }
}

// ==================== CULOARE OVERLAY ====================
static uint16_t getOverlayColor(bool haveNewData) {
    if (!haveNewData) return 0xF800; // COLOR_RED
    return 0xFD20; // COLOR_ORANGE
}

// ==================== RESTAURARE PAGINA PRINCIPALA SERVICE ====================
static void restoreServiceMain() {
    SharedPanel panel;
    if (!ApplicationState::read(panel)) return;

    Graphics::clearTargetScreen(GraphicsTarget::Panel);
    PanelRenderer::invalidate(GraphicsTarget::Panel);

    PanelRenderer::render(GraphicsTarget::Panel, panel.lift1, "ASCENSOR");

    uint16_t color = getOverlayColor(true);
    Graphics::drawServiceOverlay(GraphicsTarget::Panel, color);
}

// ==================== HANDLERE COMENZI ====================
static void handleCommand(const ServiceProtocol::Command &cmd) {
    switch (cmd.type) {
        case ServiceProtocol::Command::Type::EnterService:
            if (sInService.load(std::memory_order_relaxed)) break;
            sInService.store(true, std::memory_order_relaxed);
            sServicePageMode = ServicePageMode::Live;
            Graphics::setServiceMode(true);
            char panelId[16];
            snprintf(panelId, sizeof(panelId), "%c_%02u_%u.%02u",
                     BuildInfo::PanelType,
                     static_cast<unsigned>(BuildInfo::HardwareVariant),
                     static_cast<unsigned>(BuildInfo::Major),
                     static_cast<unsigned>(BuildInfo::Minor));
            ServiceProtocol::sendResponse(panelId);
            break;

        case ServiceProtocol::Command::Type::ExitService:
            sInService.store(false, std::memory_order_relaxed);
            sServicePageMode = ServicePageMode::Live;
            Graphics::setServiceMode(false);
            ServiceProtocol::sendResponse("ACK SRV EXIT");
            break;

        case ServiceProtocol::Command::Type::ShowCommPage:
            if (!checkService()) break;
            showCommPage();
            break;

        case ServiceProtocol::Command::Type::ResetDisplay:
            if (!checkService()) break;
            Graphics::reinitialize();
            if (sServicePageMode == ServicePageMode::ColorTest) {
                Graphics::drawColorTestPage(GraphicsTarget::Panel, sColorTestIndex, sColorTestLabelAtTop);
            } else if (sServicePageMode == ServicePageMode::Communication) {
                SharedPanel panel;
                if (!ApplicationState::read(panel)) {
                    sServicePageMode = ServicePageMode::Live;
                    PanelRenderer::invalidate(GraphicsTarget::Panel);
                    ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
                    break;
                }
                drawCommPage(panel);
            } else if (sServicePageMode == ServicePageMode::RxCounters) {
                SharedPanel panel;
                if (!ApplicationState::read(panel)) {
                    sServicePageMode = ServicePageMode::Live;
                    PanelRenderer::invalidate(GraphicsTarget::Panel);
                    ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
                    break;
                }
                drawRxCounterPage(panel);
            } else {
                restoreServiceMain();
            }
            ServiceProtocol::sendResponse("ACK_31 1");
            break;

        case ServiceProtocol::Command::Type::StartDisplayTest:
            if (!checkService()) break;
            startColorTest(cmd.param);
            break;

        case ServiceProtocol::Command::Type::StopDisplayTest:
            if (!checkService()) break;
            stopColorTest();
            ServiceProtocol::sendResponse("ACK_33");
            break;

        case ServiceProtocol::Command::Type::HideCommPage:
            if (!checkService()) break;
            closeCommPage();
            break;

        case ServiceProtocol::Command::Type::StartCommCounterDisplay: {
            if (!checkService()) break;
            if (cmd.param != 1) {
                ServiceProtocol::sendResponse("ERR 08 INVALID_PARAM");
                break;
            }
            SharedPanel panel;
            if (!ApplicationState::read(panel)) {
                ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
                break;
            }
            drawRxCounterPage(panel);
            ServiceProtocol::sendResponse("ACK_61 1");
            break;
        }

        case ServiceProtocol::Command::Type::StopCommCounterDisplay:
            if (!checkService()) break;
            if (cmd.param != 1) {
                ServiceProtocol::sendResponse("ERR 08 INVALID_PARAM");
                break;
            }
            if (sServicePageMode == ServicePageMode::RxCounters) {
                sServicePageMode = ServicePageMode::Live;
                restoreServiceMain();
            }
            ServiceProtocol::sendResponse("ACK_62 1");
            break;

        case ServiceProtocol::Command::Type::QueryCommunicationDisplay:
            if (!checkService()) break;
            ServiceProtocol::sendResponse(sServicePageMode == ServicePageMode::RxCounters
                ? "OK COMM DISPLAY 1"
                : "OK COMM DISPLAY 0");
            break;

        case ServiceProtocol::Command::Type::QueryCommunicationCounters:
            if (!checkService()) break;
            ServiceProtocol::sendResponse(ElevatorService::commCountIsEnabled()
                ? "OK COMM COUNTERS 1"
                : "OK COMM COUNTERS 0");
            break;

        case ServiceProtocol::Command::Type::QuerySdAudioFiles: {
            if (!SDCard::scanAudioFiles()) {
                ServiceProtocol::sendResponse("ERR 05 SD_NOT_READY");
                break;
            }

            char response[96];
            const uint8_t count = SDCard::audioFileCount();
            snprintf(response, sizeof(response), "OK SD COUNT %u", static_cast<unsigned>(count));
            ServiceProtocol::sendResponse(response);
            for (uint8_t index = 0; index < count; index++) {
                SDCard::AudioFileInfo info{};
                if (!SDCard::audioFileInfo(index, info)) continue;
                snprintf(response, sizeof(response), "OK SD FILE %02u %s %lu",
                         static_cast<unsigned>(info.id), info.name,
                         static_cast<unsigned long>(info.size));
                ServiceProtocol::sendResponse(response);
            }
            break;
        }

        case ServiceProtocol::Command::Type::PlaySdAudioFile: {
            char response[32];
            switch (SDCard::playAudioFile(cmd.param)) {
                case SDCard::PlayResult::Started:
                    snprintf(response, sizeof(response), "ACK_83 %02u", static_cast<unsigned>(cmd.param));
                    ServiceProtocol::sendResponse(response);
                    break;
                case SDCard::PlayResult::NotReady:
                    ServiceProtocol::sendResponse("ERR 05 SD_NOT_READY");
                    break;
                case SDCard::PlayResult::NotFound:
                    ServiceProtocol::sendResponse("ERR 05 SD_FILE_NOT_FOUND");
                    break;
                case SDCard::PlayResult::DuplicateId:
                    ServiceProtocol::sendResponse("ERR 05 SD_DUPLICATE_ID");
                    break;
                case SDCard::PlayResult::InvalidWav:
                    ServiceProtocol::sendResponse("ERR 05 SD_BAD_WAV");
                    break;
                case SDCard::PlayResult::Busy:
                    ServiceProtocol::sendResponse("ERR 02 BUSY");
                    break;
            }
            break;
        }

        case ServiceProtocol::Command::Type::McuUptime:
            if (!checkService()) break;
            sendMcuUptime();
            break;

        case ServiceProtocol::Command::Type::McuLastReset:
            if (!checkService()) break;
            sendMcuLastReset();
            break;

        case ServiceProtocol::Command::Type::McuTemp:
            if (!checkService()) break;
            sendMcuTemp();
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

// ==================== COMUNICATION ====================
static void showCommPage() {
    SharedPanel panel;
    if (!ApplicationState::read(panel)) {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
        return;
    }

    drawCommPage(panel);
    ServiceProtocol::sendResponse("ACK_66 1");
}

static void drawCommPage(const SharedPanel &panel) {
    sServicePageMode = ServicePageMode::Communication;
    Graphics::clearTargetScreen(GraphicsTarget::Panel);

    const GraphicsTarget panelDisplay = GraphicsTarget::Panel;
    char liftPos[4];
    char liftDst[4];

    Graphics::printMenuHeader(panelDisplay, "Comm. Ascensor", 0xFFE0, 2);

    Graphics::printCommLine(panelDisplay, 0, "Pos :", floorText(panel.lift1.pos, liftPos, sizeof(liftPos)), 0xFFFF);
    Graphics::printCommLine(panelDisplay, 1, "Dst :", panel.lift1.sj == Direction::Idle ? "--" : floorText(panel.lift1.etd, liftDst, sizeof(liftDst)), 0xFFFF);
    Graphics::printCommLine(panelDisplay, 2, "S/J :", directionText(panel.lift1.sj), 0xFFFF);
    Graphics::printCommLine(panelDisplay, 3, "Svc :", serviceText(panel.lift1.svc), 0xFFFF);

    bool svc1Normal = (panel.lift1.svc == ServiceState::Normal);
    Graphics::printCommLine(panelDisplay, 4, "Ocp :",
        svc1Normal ? occupancyText(panel.lift1.ocp) : "- ! -",
        svc1Normal ? 0xFFFF : 0xFE60);
}

static void drawRxCounterPage(const SharedPanel &panel) {
    static const char *labels[] = {"Total", "Valid", "Timeout", "Format", "CRC", "Data"};
    sServicePageMode = ServicePageMode::RxCounters;
    Graphics::clearTargetScreen(GraphicsTarget::Panel);
    sRxCounterValuesDirty = true;

    const int16_t labelX = DisplayGeom::screenW() * 3 / 100;
    for (uint8_t row = 0; row < 6; row++) {
        const int16_t baseline = DisplayGeom::screenH() * (22 + row * 13) / 100;
        Graphics::drawText(GraphicsTarget::Panel, labelX, baseline,
                           &FreeSans12pt7b, labels[row], 0xFFFF);
    }
    renderRxCounterValues(panel);
}

static void renderRxCounterValues(const SharedPanel &panel) {
    const CommLineCounters &counters = panel.comm.lift1;
    const uint32_t values[] = {
        counters.rxFrames,
        counters.rxValid,
        counters.rxTimeout,
        counters.rxFormatError,
        counters.rxCrcError,
        counters.rxDataError
    };
    const int16_t valueX = DisplayGeom::screenW() * 46 / 100;

    for (uint8_t row = 0; row < 6; row++) {
        if (sRxCounterValuesDirty || sLastRxCounterValues[row] != values[row]) {
            char text[11];
            if (!sRxCounterValuesDirty) {
                char oldText[11];
                snprintf(oldText, sizeof(oldText), "%lu",
                         static_cast<unsigned long>(sLastRxCounterValues[row]));
                const int16_t oldBaseline = DisplayGeom::screenH() * (22 + row * 13) / 100;
                Graphics::drawText(GraphicsTarget::Panel, valueX, oldBaseline,
                                   &FreeSans12pt7b, oldText, 0x0000);
            }
            snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(values[row]));
            const int16_t baseline = DisplayGeom::screenH() * (22 + row * 13) / 100;
            Graphics::drawText(GraphicsTarget::Panel, valueX, baseline,
                               &FreeSans12pt7b, text, 0x07E0);
            sLastRxCounterValues[row] = values[row];
        }
    }
    sRxCounterValuesDirty = false;
}

static void startColorTest(uint8_t target) {
    if (target != 1) {
        ServiceProtocol::sendResponse("ERR 08 INVALID_PARAM");
        return;
    }

    sServicePageMode = ServicePageMode::ColorTest;
    sColorTestIndex = 0;
    sColorTestLabelAtTop = true;
    sColorTestLastStepMillis = millis();
    Graphics::drawColorTestPage(GraphicsTarget::Panel, sColorTestIndex, sColorTestLabelAtTop);
    ServiceProtocol::sendResponse("ACK_32 1");
}

static void stopColorTest() {
    if (sServicePageMode != ServicePageMode::ColorTest) return;
    sServicePageMode = ServicePageMode::Live;
    restoreServiceMain();
}

static void updateColorTest() {
    if (millis() - sColorTestLastStepMillis < COLOR_TEST_STEP_MS) return;

    sColorTestLastStepMillis = millis();
    sColorTestIndex++;
    if (sColorTestIndex >= 5) {
        sColorTestIndex = 0;
        sColorTestLabelAtTop = !sColorTestLabelAtTop;
    }
    Graphics::drawColorTestPage(GraphicsTarget::Panel, sColorTestIndex, sColorTestLabelAtTop);
}

static void closeCommPage() {
    sServicePageMode = ServicePageMode::Live;
    restoreServiceMain();
    ServiceProtocol::sendResponse("ACK_66 0");
}

// ==================== PICO / MCU (Serial only) ====================
static void sendMcuUptime() {
    SharedPanel localPanel;
    if (ApplicationState::read(localPanel)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "OK UPTIME %lu", localPanel.system.uptimeSeconds);
        ServiceProtocol::sendResponse(buf);
    } else {
        ServiceProtocol::sendResponse("ERR 06 SEQLOCK");
    }
}

static void sendMcuLastReset() {
    SharedPanel localPanel;
    if (ApplicationState::read(localPanel)) {
        const char* reason = "UNKNOWN";
        switch (localPanel.system.lastResetReason) {
            case 1: reason = "POR_BROWNOUT"; break;
            case 2: reason = "WDT"; break;
            case 3: reason = "SYSREQ"; break;
            case 4: reason = "4PUSH"; break;
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
    const float temperatureC = analogReadTemp();
    if (!std::isfinite(temperatureC)) {
        ServiceProtocol::sendResponse("ERR 10 TEMP_READ_FAILED");
        return;
    }

    const long centiDegrees = std::lround(temperatureC * 100.0f);
    const unsigned long magnitude = static_cast<unsigned long>(
        centiDegrees < 0 ? -centiDegrees : centiDegrees);
    char buf[64];
    snprintf(buf, sizeof(buf), "OK TEMP %s%lu.%02lu C",
             centiDegrees < 0 ? "-" : "",
             magnitude / 100,
             magnitude % 100);
    ServiceProtocol::sendResponse(buf);
}

} // namespace ServiceMenu