// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    void init();
    void update(SharedPanel &localPanel);

    // Contoare de recepție encoder (controlate din Service Box, inactive după reboot)
    void commCountEnable();   // porneste numararea
    void commCountDisable();  // opreste numararea SI zeroizeaza contoarele
    void commCountReset();    // zeroizeaza contoarele, starea ON/OFF ramane neschimbata
    bool commCountIsEnabled();
}

#endif // PROTOCOL_H
