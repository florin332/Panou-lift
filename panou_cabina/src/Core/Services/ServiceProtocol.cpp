#include "ServiceProtocol.h"
#include "ElevatorService.h"
#include "ConfigurationService.h"
#include "SystemCounters.h"
#include "AlarmService.h"
#include "../../Presentation/ServiceMenu.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <pico/critical_section.h>

namespace ServiceProtocol {

static Command sPendingCmd;
static ResponseSink sResponseSink = nullptr;
static critical_section_t sCommandLock;

static void lockCommand() {
    critical_section_enter_blocking(&sCommandLock);
}

static void unlockCommand() {
    critical_section_exit(&sCommandLock);
}

static void parseLine(const char* line);

// newlib-nano (RP2040) nu are %f în snprintf implicit — formatăm manual X.XX
static void formatGain(char* out, size_t len, float gain) {
    if (gain < 0.0f) gain = 0.0f;
    unsigned long centi = static_cast<unsigned long>(gain * 100.0f + 0.5f);
    snprintf(out, len, "%lu.%02lu", centi / 100UL, centi % 100UL);
}

// Argumentele numerice se transmit simplu, fără markere între paranteze.
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
    critical_section_init(&sCommandLock);
    lockCommand();
    sPendingCmd = Command{};
    unlockCommand();
}

void setResponseSink(ResponseSink sink) { sResponseSink = sink; }

void submitLine(const char* line) {
    if (line != nullptr) parseLine(line);
}

static void parseLine(const char* line) {
    while (*line == ' ') line++;

    Command cmd{};
    if (strcmp(line, "SRV ENTER") == 0 || strcmp(line, "mb_01") == 0) {
        cmd.type = Command::Type::EnterService;
    } else if (strcmp(line, "SRV EXIT") == 0 || strcmp(line, "mb_00") == 0) {
        cmd.type = Command::Type::ExitService;
    } else if (strcmp(line, "mb_02") == 0) {
        // Reset in bootloader USB (volum MSC "RPI-RP2") pentru flash-uire
        // de pe Service Box. Executia este amanata ca comanda si ruleaza pe
        // core0 (ServiceMenu::handleCommand) - TinyUSB nu este thread-safe.
        // Markerul P0 dovedeste in logul box-ului ca comanda a AJUNS pe panou
        // (parseLine ruleaza acum tot pe core0, sendResponse e sigur aici).
        sendResponse("DBG P0 bootsel-primit");
        cmd.type = Command::Type::EnterBootsel;
    } else if (strncmp(line, "mb_66", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        const long value = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || (value != 0 && value != 1)) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = value == 1 ? Command::Type::ShowCommPage : Command::Type::HideCommPage;
        cmd.param = static_cast<uint8_t>(value);
    } else if (strncmp(line, "mb_31", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        const long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::ResetDisplay;
        cmd.param = 1;
    } else if (strncmp(line, "mb_32", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        const long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
        if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StartDisplayTest;
        cmd.param = 1;
    } else if (strcmp(line, "mb_33") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::StopDisplayTest;
    } else if (strncmp(line, "mb_61", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        const long value = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || value != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StartCommCounterDisplay;
        cmd.param = 1;
    } else if (strncmp(line, "mb_62", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        const long value = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || value != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        cmd.type = Command::Type::StopCommCounterDisplay;
        cmd.param = 1;
    } else if (strncmp(line, "mb_63", 5) == 0 && (line[5] == ' ')) {
        long target = 0;
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        target = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || target != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        ElevatorService::commCountEnable();
        sendResponse("ACK_63 1");
        return;
    } else if (strncmp(line, "mb_64", 5) == 0 && (line[5] == ' ')) {
        long target = 0;
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        target = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || target != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        ElevatorService::commCountDisable();
        sendResponse("ACK_64 1");
        return;
    } else if (strncmp(line, "mb_65", 5) == 0 && (line[5] == ' ')) {
        long target = 0;
        if (!checkServiceMode()) return;
        const char* arg = numericArg(line + 5);
        char* endPtr = nullptr;
        target = strtol(arg, &endPtr, 10);
        if (endPtr == arg || *endPtr != '\0' || target != 1) {
            sendResponse("ERR 08 INVALID_PARAM");
            return;
        }
        ElevatorService::commCountReset();
        sendResponse("ACK_65 1");
        return;
    } else if (strcmp(line, "mb_67") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::QueryCommunicationDisplay;
    } else if (strcmp(line, "mb_68") == 0) {
        if (!checkServiceMode()) return;
        cmd.type = Command::Type::QueryCommunicationCounters;
    } else if (strcmp(line, "mb_45") == 0) {
        cmd.type = Command::Type::McuUptime;
    } else if (strcmp(line, "mb_47") == 0) {
        cmd.type = Command::Type::McuLastReset;
    } else if (strcmp(line, "mb_46") == 0) {
        cmd.type = Command::Type::McuTemp;
    } else if (strncmp(line, "mb_4", 4) == 0 && line[4] >= '1' && line[4] <= '4' &&
               (line[5] == '\0' || line[5] == ' ')) {
        if (!checkServiceMode()) return;
        const uint8_t category = static_cast<uint8_t>(line[4] - '0');
        char response[48];
        if (line[5] == '\0') {
            uint32_t counts[4];
            SystemCounters::readResetCounts(counts);
            snprintf(response, sizeof(response), "OK COUNTER %u %lu",
                     static_cast<unsigned>(category),
                     static_cast<unsigned long>(counts[category - 1]));
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            const long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : -1;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value != 0) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            if (!SystemCounters::resetCategory(category)) {
                sendResponse("ERR 09 EEPROM_WRITE_FAILED");
                return;
            }
            snprintf(response, sizeof(response), "ACK_4%u 0", static_cast<unsigned>(category));
        }
        sendResponse(response);
        return;
    } else if (strncmp(line, "mb_11", 5) == 0 && (line[5] == '\0' || line[5] == ' ' || line[5] == '<')) {
        // Număr ascensor persistent (flash protejat)
        if (!checkServiceMode()) return;
        char resp[48];
        if (line[5] == '\0') {
            snprintf(resp, sizeof(resp), "OK LIFT %u", ConfigurationService::liftNumber());
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            long value = (arg != nullptr) ? strtol(arg, &endPtr, 10) : 0;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || value < 1 || value > 65535) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            ConfigurationService::setLiftNumber(static_cast<uint16_t>(value));
            snprintf(resp, sizeof(resp), "ACK_11 %u", ConfigurationService::liftNumber());
        }
        sendResponse(resp);
        return;
    } else if (strncmp(line, "mb_12", 5) == 0 &&
               (line[5] == '\0' || line[5] == ' ' || line[5] == '<' ||
                (line[5] >= '0' && line[5] <= '9') || line[5] == '.')) {
        // Gain audio persistent (flash protejat), aplicat live pe I2S
        if (!checkServiceMode()) return;
        char resp[48];
        char gainStr[12];
        if (line[5] == '\0') {
            formatGain(gainStr, sizeof(gainStr), AlarmService::gain());
            snprintf(resp, sizeof(resp), "OK GAIN %s", gainStr);
        } else {
            const char* arg = numericArg(line + 5);
            char* endPtr = nullptr;
            float value = (arg != nullptr) ? strtof(arg, &endPtr) : 0.0f;
            if (arg == nullptr || endPtr == arg || *endPtr != '\0' || !AlarmService::setGain(value)) {
                sendResponse("ERR 08 INVALID_PARAM");
                return;
            }
            formatGain(gainStr, sizeof(gainStr), AlarmService::gain());
            snprintf(resp, sizeof(resp), "ACK_12 %s", gainStr);
        }
        sendResponse(resp);
        return;
    } else if (strcmp(line, "mb_30") == 0) {
        // Revine la default-urile din Config.h (lift + gain) și persistă
        if (!checkServiceMode()) return;
        ConfigurationService::resetDefaults();
        if (AlarmService::isPlaying()) AlarmService::stop();
        AlarmService::setGain(ConfigurationService::audioGain());
        sendResponse("ACK_30");
        return;
    } else {
        sendResponse("ERR 01 UNKNOWN_CMD");
        return;
    }

    lockCommand();
    if (!sPendingCmd.pending) {
        sPendingCmd = cmd;
        sPendingCmd.pending = true;
        unlockCommand();
    } else {
        unlockCommand();
        sendResponse("ERR 02 BUSY");
    }
}

bool consumeCommand(Command &out) {
    lockCommand();
    if (!sPendingCmd.pending) {
        unlockCommand();
        return false;
    }
    out = sPendingCmd;
    sPendingCmd.pending = false;
    unlockCommand();
    return true;
}

void sendResponse(const char* response) {
    if (sResponseSink != nullptr) sResponseSink(response);
}

bool hasPendingCommand() {
    lockCommand();
    const bool pending = sPendingCmd.pending;
    unlockCommand();
    return pending;
}

} // namespace ServiceProtocol