// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    void init();
    void update(SharedPanel &localPanel);
    void trimiteApel(uint8_t ascAlocat);

    // Contoare receptie seriala detaliate (controlate din Service Box, inactive dupa reboot)
    void commCountEnable();   // porneste numararea
    void commCountDisable();  // opreste numararea SI zeroizeaza contoarele
    void commCountReset();    // zeroizeaza contoarele, starea ON/OFF ramane neschimbata
    bool commCountIsEnabled();

    // Forward cadre brute RX catre Service Box pe Serial USB
    void commAnalyzeEnable();
    void commAnalyzeDisable();
}

#endif // PROTOCOL_H
