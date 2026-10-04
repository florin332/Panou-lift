#include "ServiceProtocol.h"
#include "Protocol.h"
#include "../../Presentation/ServiceMenu.h"
#include <Arduino.h>
#include <cstring>

namespace ServiceProtocol {

static Command sPendingCmd;
static char sRxBuffer[64];
static uint8_t sRxIndex = 0;

static void parseLine(const char* line);

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
    if (strcmp(line, "SRV ENTER") == 0 || strcmp(line, "mb_in") == 0) {
        cmd.type = Command::Type::EnterService;
    } else if (strcmp(line, "SRV EXIT") == 0 || strcmp(line, "mb_out") == 0) {
        cmd.type = Command::Type::ExitService;
    } else if (strcmp(line, "COMM TEST") == 0 || strcmp(line, "mb_com") == 0) {
        cmd.type = Command::Type::CommTest;
    } else if (strcmp(line, "mb_com_out") == 0) {
        cmd.type = Command::Type::CommTestOut;
    } else if (strcmp(line, "mb_status") == 0) {
        cmd.type = Command::Type::CommStatus;
    } else if (strcmp(line, "mb_diag") == 0) {
        cmd.type = Command::Type::Diagnostics;
    } else if (strncmp(line, "DISP TEST ", 10) == 0 || strncmp(line, "mb_display_test ", 16) == 0) {
        cmd.type = Command::Type::DispTest;
        cmd.param = static_cast<uint8_t>(atoi(line[1] == 'b' ? line + 16 : line + 10));
    } else if (strncmp(line, "DISP REINIT ", 12) == 0 || strncmp(line, "mb_display_reinit ", 19) == 0) {
        cmd.type = Command::Type::DispReinit;
        const char* argument = line[1] == 'b' ? line + 19 : line + 12;
        cmd.param = strcmp(argument, "BOTH") == 0 ? 3 : static_cast<uint8_t>(atoi(argument));
    } else if (strcmp(line, "TEST EXIT") == 0 || strcmp(line, "mb_test_out") == 0) {
        cmd.type = Command::Type::DispTestExit;
    } else if (strcmp(line, "MCU UPTIME") == 0 || strcmp(line, "mb_runtime") == 0) {
        cmd.type = Command::Type::McuUptime;
    } else if (strcmp(line, "MCU RESETS") == 0 || strcmp(line, "mb_resets") == 0) {
        cmd.type = Command::Type::McuResets;
    } else if (strcmp(line, "MCU WDT") == 0 || strcmp(line, "mb_wdt") == 0) {
        cmd.type = Command::Type::McuWdt;
    } else if (strcmp(line, "MCU LASTRESET") == 0 || strcmp(line, "mb_lastreset") == 0) {
        cmd.type = Command::Type::McuLastReset;
    } else if (strcmp(line, "MCU TEMP") == 0 || strcmp(line, "mb_temp") == 0) {
        cmd.type = Command::Type::McuTemp;
    } else if (strncmp(line, "MCU STACK ", 10) == 0 || strncmp(line, "mb_stack ", 9) == 0) {
        cmd.type = Command::Type::McuStack;
        cmd.param = static_cast<uint8_t>(atoi(line[1] == 'b' ? line + 9 : line + 10));
    } else if (strcmp(line, "comm_count_enable") == 0) {
        if (!checkServiceMode()) return;
        Protocol::commCountEnable();
        sendResponse("ACK COMM COUNT ON");
        return;
    } else if (strcmp(line, "comm_count_disable") == 0) {
        if (!checkServiceMode()) return;
        Protocol::commCountDisable();
        sendResponse("ACK COMM COUNT OFF");
        return;
    } else if (strcmp(line, "comm_count_reset") == 0) {
        if (!checkServiceMode()) return;
        Protocol::commCountReset();
        sendResponse("ACK COMM COUNT RESET");
        return;
    } else if (strcmp(line, "com_analyze_enable") == 0) {
        if (!checkServiceMode()) return;
        Protocol::commAnalyzeEnable();
        sendResponse("ACK COM ANALYZE ON");
        return;
    } else if (strcmp(line, "com_analyze_disable") == 0) {
        if (!checkServiceMode()) return;
        Protocol::commAnalyzeDisable();
        sendResponse("ACK COM ANALYZE OFF");
        return;
    } else if (strcmp(line, "mb_com_status") == 0) {
        cmd.type = Command::Type::CommCountStatus;
    } else if (strcmp(line, "mb_com_status_out") == 0) {
        // Fara pagina pe panou (afisarea e pe Service Box): ACK no-op
        sendResponse("ACK COMM STATUS OUT");
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