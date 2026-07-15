// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    // Inițializează porturile fizice (Nivelul: Transport)
    void init();

    // Task-ul principal asincron de pe Core 1 (Nivelul: Receiver - Etapa 1)
    void update(SharedPanel &localPanel);

    // Trimite fizic pachetul spre fire (Nivelul: Transport)
    void trimiteApel(uint8_t ascAlocat);

    // Funcție izolată pură de testare (Nivelul: Parser - Etapa 2)
    // Transformă un string brut binar de tip "<1,12,0,5,0>" într-o structură completă LiftState
    bool parseazaPachet(const char* sirBrut, LiftState &rezultat);
}

#endif // PROTOCOL_H
