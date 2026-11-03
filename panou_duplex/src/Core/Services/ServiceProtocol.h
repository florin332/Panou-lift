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
        ResetDisplay,     // mb_31 - LCD-ul vizat este în argument
        StartDisplayTest, // mb_32 - LCD-ul vizat este în argument
        StopDisplayTest,  // mb_33 - oprește testul pe toate LCD-urile
        EnterCommunicationDisplay, // mb_61 <x>
        ExitCommunicationDisplay,  // mb_62 <x>
        StartCommunicationCounters, // mb_63 <x>
        StopCommunicationCounters,  // mb_64 <x>
        EnablePassThrough,          // mb_81
        DisablePassThrough          // mb_82
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