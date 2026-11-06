#include "ServiceProtocol.h"
#include "Settings.h"
#include "SystemCounters.h"
#include "Config.h"
#include "Protocol.h"
#include "SharedPanel.h"
#include "../../Presentation/ServiceMenu.h"
#include <Arduino.h>
#include <pico/bootrom.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace ServiceProtocol {

static Command sPendingCmd;
static char sRxBuffer[64];
static uint8_t sRxIndex = 0;
static char sRawFrameBuffer[40];
static uint8_t sRawFrameIndex = 0;
static bool sReceivingRawFrame = false;
static bool sRawFrameOverflow = false;

static void parseLine(const char* line);

// Argumentele numerice folosesc forma simpla "mb_11 3".
static const char* numericArg(const char* raw) {
    while (*raw == ' ') raw++;
    return raw;
}

static bool checkServiceMode() {
    if (!ServiceMenu::isInServiceMode()) {
        sendResponse("ERR 03 NOT_IN_SERVICE");
        return false;
    }
    return true;
}

void init() {
    sRxIndex = 0;
    sRawFrameIndex = 0;
    sReceivingRawFrame = false;
    sRawFrameOverflow = false;
    sPendingCmd = Command{};
    Protocol::setPassThroughEnabled(false);
}

static void handleRawFrame() {
    if (!ServiceMenu::isInServiceMode()) {
        sendResponse("ERR 03 NOT_IN_SERVICE");
    } else if (!Protocol::isPassThroughEnabled()) {
        sendResponse("ERR 11 PASSTHROUGH_DISABLED");
    } else if (!Protocol::queuePassThroughFrame(sRawFrameBuffer, sRawFrameIndex)) {
        sendResponse("ERR 02 BUSY");
    } else {
        sendResponse("ACK_PT");
    }
    sRawFrameIndex = 0;
}

void poll() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());

        if (sReceivingRawFrame) {
            if (sRawFrameOverflow) {
                if (c == '>') {
                    sReceivingRawFrame = false;
                    sRawFrameOverflow = false;
                    sRawFrameIndex = 0;
                    sendResponse("ERR 08 INVALID_PARAM");
                }
                continue;
            }
            if (c == '\n' || c == '\r') {
                sReceivingRawFrame = false;
                sRawFrameIndex = 0;
                sendResponse("ERR 08 INVALID_PARAM");
                continue;
            }
            if (sRawFrameIndex >= sizeof(sRawFrameBuffer) - 1) {
                sRawFrameOverflow = true;
                if (c == '>') {
                    sReceivingRawFrame = false;
                    sRawFrameOverflow = false;
                    sRawFrameIndex = 0;
                    sendResponse("ERR 08 INVALID_PARAM");
                }
                continue;
            }
            sRawFrameBuffer[sRawFrameIndex++] = c;
            if (c == '>') {
                sReceivingRawFrame = false;
                handleRawFrame();
            }
            continue;
        }

        if (sRxIndex == 0 && c == '<') {
            sReceivingRawFrame = true;
            sRawFrameIndex = 0;
            sRawFrameOverflow = false;
            sRawFrameBuffer[sRawFrameIndex++] = c;
            continue;
        }

        if (c == '\n' || c == '\r') {
            if (sRxIndex > 0) {
                sRxBuffer[sRxIndex] = '\0';
                parseLine(sRxBuffer);
                sRxIndex = 0;
            }
        } else if (sRxIndex < sizeof(sRxBuffer) - 1) {
            sRxBuffer[sRxIndex++] = c;
        }
    }
}

static void parseLine(const char* line) {
    while (*line == ' ') line++;

    Command cmd{};
    if (strcmp(line, "mb_01") == 0) {
        cmd.type = Command::Type::EnterService;
    } else if (strcmp(line, "mb_00") == 0) {
        cmd.type = Command::Type::ExitService;
    } else if (strcmp(line, "mb_02") == 0) {
        // Reset in bootloader USB (volum MSC "RPI-RP2") pentru flash-uire
        // de pe Service Box. Executia este amanata ca comanda si ruleaza pe
        // core0 (ServiceMenu::handleCommand) - TinyUSB nu este thread-safe.
        // Markerul P0 dovedeste in logul box-ului ca comanda a AJUNS pe panou
        // (parseLine ruleaza acum tot pe core0, sendResponse e sigur aici).
        sendResponse("DBG P0 bootsel-primit");
        cmd.type = Command::Type::EnterBootsel;
    } else if (strcmp(line, "mb_81") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::EnablePassThrough;
    } else if (strcmp(line, "mb_82") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::DisablePassThrough;
    } else if (strncmp(line, "mb_31", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::ResetDisplay;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_32", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StartDisplayTest;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strcmp(line, "mb_33") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::StopDisplayTest;
    } else if (strncmp(line, "mb_61", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::EnterCommunicationDisplay;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_62", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::ExitCommunicationDisplay;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_63", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StartCommunicationCounters;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_64", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StopCommunicationCounters;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_65", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 3) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::ResetCommunicationCounters;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strcmp(line, "mb_67") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::QueryCommunicationDisplay;
    } else if (strcmp(line, "mb_68") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::QueryCommunicationCounters;
    } else if (strncmp(line, "mb_11", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // Etaj panou (Etj) persistent (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK ETJ %u", Settings::panelFloor());
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' ||
                value < 0 || value >= Config::Hardware::FLOORS) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            Settings::setPanelFloor(static_cast<uint8_t>(value));
            snprintf(resp, sizeof(resp), "ACK_11 %u", Settings::panelFloor());
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_13", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // Permisiune al doilea apel (SecondCall) persistentă (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK SECOND CALL %u", Settings::secondCall() ? 1u : 0u);
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || (value != 0 && value != 1)) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            Settings::setSecondCall(value == 1);
            snprintf(resp, sizeof(resp), "ACK_13 %ld", value);
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_14", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // Timeout standby ecran persistent (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK STANDBY %lu", static_cast<unsigned long>(Settings::standbyTimeoutMs()));
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' ||
                !Settings::setStandbyTimeoutMs(static_cast<uint32_t>(value))) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            snprintf(resp, sizeof(resp), "ACK_14 %lu", static_cast<unsigned long>(Settings::standbyTimeoutMs()));
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_15", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // Ordine cabine pe LCD-uri (CabOrder) persistentă (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK CAB ORDER %u", Settings::cabOrderSwapped() ? 1u : 0u);
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || (value != 0 && value != 1)) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            Settings::setCabOrderSwapped(value == 1);
            snprintf(resp, sizeof(resp), "ACK_15 %ld", value);
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_16", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // Configurația stațiilor este persistentă și poate fi setată o singură dată.
        if (!checkServiceMode()) return;
        char resp[64];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK STATIONS %u %u %u",
                     Settings::totalStations(), Settings::groundStation(),
                     Settings::stationConfigWasSet() ? 1u : 0u);
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long totalStations = strtol(arg, &endPtr, 10);
            if (endPtr == arg || *endPtr != ' ') {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            arg = numericArg(endPtr);
            long groundStation = strtol(arg, &endPtr, 10);
            if (endPtr == arg || *endPtr != '\0' || totalStations < 1 || totalStations > 30 ||
                groundStation < 0 || groundStation > 255 ||
                !Settings::setStationConfig(static_cast<uint8_t>(totalStations),
                                            static_cast<uint8_t>(groundStation))) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            snprintf(resp, sizeof(resp), "ACK_16 %ld %ld", totalStations, groundStation);
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_17", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || (value != 0 && value != 1)) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::ShowLiftAssignments;
        cmd.argument = static_cast<uint8_t>(value);
    } else if (strcmp(line, "mb_30") == 0) {
        // Revine la default-urile din Config.h (Etj + standby + secondCall + cabOrder) și persistă
        if (!checkServiceMode()) return;
        Settings::resetDefaults();
        sendResponse("ACK_30");
        return;
    } else if (strcmp(line, "mb_45") == 0) {
        if (!checkServiceMode()) return;
        SharedPanel snapshot{};
        if (!shared_panel_read(gSharedMemory, snapshot)) {
            sendResponse("ERR 02 BUSY");
            return;
        }
        char resp[48];
        snprintf(resp, sizeof(resp), "OK UPTIME %lu",
                 static_cast<unsigned long>(snapshot.system.uptimeSeconds));
        sendResponse(resp);
        return;
    } else if (strcmp(line, "mb_46") == 0) {
        if (!checkServiceMode()) return;
        const float temperatureC = analogReadTemp();
        if (!std::isfinite(temperatureC)) {
            sendResponse("ERR 10 TEMP_READ_FAILED");
            return;
        }
        const long centiDegrees = std::lround(temperatureC * 100.0f);
        const unsigned long magnitude = static_cast<unsigned long>(
            centiDegrees < 0 ? -centiDegrees : centiDegrees);
        char resp[48];
        snprintf(resp, sizeof(resp), "OK TEMP %s%lu.%02lu C",
                 centiDegrees < 0 ? "-" : "",
                 magnitude / 100,
                 magnitude % 100);
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_4", 4) == 0 && line[4] >= '1' && line[4] <= '4' &&
               (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const uint8_t category = static_cast<uint8_t>(line[4] - '0');
        char resp[96];
        if (line[5] == '\0') {
            uint32_t counts[4];
            SystemCounters::readResetCounts(counts);
            snprintf(resp, sizeof(resp), "OK COUNTER %u %lu",
                     static_cast<unsigned>(category),
                     static_cast<unsigned long>(counts[category - 1]));
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value != 0) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            if (!SystemCounters::resetCategory(category)) {
                sendResponse("ERR 09 EEPROM_WRITE_FAILED");
                return;
            }
            snprintf(resp, sizeof(resp), "ACK_4%u 0", static_cast<unsigned>(category));
        }
        sendResponse(resp);
        return;
    } else {
        sendResponse("ERR 01 UNKNOWN_CMD");
        return;
    }

    if (!sPendingCmd.pending) {
        sPendingCmd = cmd;
        sPendingCmd.pending = true;
    } else {
        sendResponse("ERR 02 BUSY");
    }
}

bool consumeCommand(Command &out) {
    if (!sPendingCmd.pending) return false;
    out = sPendingCmd;
    sPendingCmd.pending = false;
    return true;
}

void sendResponse(const char* response) {
    Serial.println(response);
}

bool hasPendingCommand() {
    return sPendingCmd.pending;
}

} // namespace ServiceProtocol