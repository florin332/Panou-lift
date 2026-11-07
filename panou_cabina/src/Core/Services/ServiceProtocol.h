#ifndef SERVICE_PROTOCOL_H
#define SERVICE_PROTOCOL_H

#include <cstdint>

namespace ServiceProtocol {

struct Command {
    enum class Type : uint8_t {
        None,
        EnterService,
        ExitService,
        EnterBootsel,     // SRV BOOTSEL / mb_boot - executat pe core0
        CommTest,
        CommTestOut,      // exit COMM test
        StartCommCounterDisplay, // mb_61 1
        StopCommCounterDisplay,  // mb_62 1
        QueryCommunicationDisplay, // mb_67
        QueryCommunicationCounters, // mb_68
        Diagnostics,
        ResetDisplay,     // mb_31 1
        StartDisplayTest, // mb_32 1
        StopDisplayTest,  // mb_33
        McuUptime,
        McuResets,
        McuWdt,
        McuLastReset,
        McuTemp
    } type = Type::None;

    uint8_t param = 0;
    bool pending = false;
};

using ResponseSink = void (*)(const char *response);

void init();
void setResponseSink(ResponseSink sink);
void submitLine(const char *line);
bool consumeCommand(Command &out);
void sendResponse(const char* response);
bool hasPendingCommand();

} // namespace ServiceProtocol

#endif