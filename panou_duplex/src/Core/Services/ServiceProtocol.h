#ifndef SERVICE_PROTOCOL_H
#define SERVICE_PROTOCOL_H

#include <cstdint>

namespace ServiceProtocol {

struct Command {
    enum class Type : uint8_t {
        None,
        EnterService,
        ExitService,
        CommTest,
        CommTestOut,      // exit COMM test
        CommStatus,
        Diagnostics,
        DispTest,
        DispReinit,
        DispTestExit,     // exit LCD test (fostul TestExit)
        McuUptime,
        McuResets,
        McuWdt,
        McuLastReset,
        McuTemp,
        McuStack
    } type = Type::None;

    uint8_t param = 0;
    bool pending = false;
};

void init();
void poll();
bool consumeCommand(Command &out);
void sendResponse(const char* response);
bool hasPendingCommand();

} // namespace ServiceProtocol

#endif