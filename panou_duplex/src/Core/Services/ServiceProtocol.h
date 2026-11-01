#ifndef SERVICE_PROTOCOL_H
#define SERVICE_PROTOCOL_H

#include <cstdint>

namespace ServiceProtocol {

struct Command {
    enum class Type : uint8_t {
        None,
        EnterService,
        ExitService,
        EnterBootsel      // mb_02 - executat pe core0
    } type = Type::None;

    bool pending = false;
};

void init();
void poll();
bool consumeCommand(Command &out);
void sendResponse(const char* response);
bool hasPendingCommand();

} // namespace ServiceProtocol

#endif