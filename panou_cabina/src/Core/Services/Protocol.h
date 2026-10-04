// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    void init();
    void update(SharedPanel &localPanel);
    void trimiteApel(uint8_t ascAlocat);
    bool parseazaPachet(const char* sirBrut, LiftState &rezultat);
}

#endif // PROTOCOL_H
