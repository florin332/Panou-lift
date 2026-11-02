#ifndef SERVICE_PROTOCOL_H
#define SERVICE_PROTOCOL_H

#include <cstdint>

namespace ServiceProtocol {

struct Command {
    enum class Type : uint8_t {
        None,
        EnterService,
        ExitService,
        EnterBootsel,     // mb_02 - executat pe core0
        ResetDisplay,     // mb_16 - LCD-ul vizat este în argument
        StartDisplayTest, // mb_17 - LCD-ul vizat este în argument
        StopDisplayTest   // mb_18 - oprește testul pe toate LCD-urile
    } type = Type::None;

    uint8_t argument = 0;
    bool pending = false;
};

void init();
void poll();
bool consumeCommand(Command &out);
void sendResponse(const char* response);
bool hasPendingCommand();

} // namespace ServiceProtocol

#endif