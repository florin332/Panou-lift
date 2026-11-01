#include "ServiceProtocol.h"
#include "Settings.h"
#include "Config.h"
#include "../../Presentation/ServiceMenu.h"
#include <Arduino.h>
#include <pico/bootrom.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace ServiceProtocol {

static Command sPendingCmd;
static char sRxBuffer[64];
static uint8_t sRxIndex = 0;

static void parseLine(const char* line);

// Extrage argumentul numeric: acceptă fie text simplu ("mb_11 3"),
// fie încadrat de markerele protocolului ("mb_11 <3>" / "mb_11<3>").
static const char* numericArg(const char* raw, char* scratch, size_t scratchLen) {
    while (*raw == ' ') raw++;
    if (*raw != '<') return raw;
    const char* end = strchr(raw + 1, '>');
    if (end == nullptr) return nullptr;
    size_t n = static_cast<size_t>(end - (raw + 1));
    if (n == 0 || n >= scratchLen) return nullptr;
    memcpy(scratch, raw + 1, n);
    scratch[n] = '\0';
    return scratch;
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
    sPendingCmd = Command{};
}

void poll() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
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
    } else if (strncmp(line, "mb_11", 5) == 0 && (line[5] == '\0' || line[5] == ' ' || line[5] == '<')) {
        // Etaj panou (Etj) persistent (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK ETJ %u", Settings::panelFloor());
        } else {
            char scratch[16];
            const char* arg = numericArg(line + 5, scratch, sizeof(scratch));
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
    } else if (strncmp(line, "mb_13", 5) == 0 && (line[5] == '\0' || line[5] == ' ' || line[5] == '<')) {
        // Permisiune al doilea apel (SecondCall) persistentă (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK SECOND CALL %u", Settings::secondCall() ? 1u : 0u);
        } else {
            char scratch[16];
            const char* arg = numericArg(line + 5, scratch, sizeof(scratch));
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
    } else if (strncmp(line, "mb_14", 5) == 0 && (line[5] == '\0' || line[5] == ' ' || line[5] == '<')) {
        // Timeout standby ecran persistent (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK STANDBY %lu", static_cast<unsigned long>(Settings::standbyTimeoutMs()));
        } else {
            char scratch[16];
            const char* arg = numericArg(line + 5, scratch, sizeof(scratch));
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
    } else if (strcmp(line, "mb_30") == 0) {
        // Revine la default-urile din Config.h (Etj + standby + secondCall) și persistă
        if (!checkServiceMode()) return;
        Settings::resetDefaults();
        sendResponse("ACK_30");
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