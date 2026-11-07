// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    void init();
    void update(SharedPanel &localPanel);

    // Contoare RX de encoder, inactive după reboot.
    void commCountEnable();   // porneste numararea
    void commCountDisable();  // opreste numararea, pastrand valorile
    void commCountReset();    // cere resetarea pe Core 1, starea ON/OFF ramane neschimbata
    bool commCountIsEnabled();
}

#endif // PROTOCOL_H
