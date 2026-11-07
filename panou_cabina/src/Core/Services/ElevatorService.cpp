#include "ElevatorService.h"
#include "Protocol.h"

namespace ElevatorService
{
    void init() {
        Protocol::init();
    }

    void update(SharedPanel &state) {
        Protocol::update(state);
    }

    void commCountEnable() { Protocol::commCountEnable(); }
    void commCountDisable() { Protocol::commCountDisable(); }
    void commCountReset() { Protocol::commCountReset(); }
    bool commCountIsEnabled() { return Protocol::commCountIsEnabled(); }
}