// Drivers/Protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "SharedPanel.h"

namespace Protocol
{
    void init();
    void update(SharedPanel &localPanel);

    // Transmisie imediată, fără carrier sense (uz intern / Service Box)
    void trimiteApel(uint8_t ascAlocat);

    // Cerere de apel cu carrier sense (CSMA): apelul este pus în așteptare și
    // transmis de update() când linia de comandă e liberă (vezi Config::Protocol)
    void requestCall(uint8_t ascAlocat);

    // Contoare RX volatile, controlate separat pentru lifturile 1 si 2.
    bool commCountEnable(uint8_t lift);   // lift 1 sau 2
    bool commCountDisable(uint8_t lift); // opreste fara sa stearga valorile
    bool commCountReset(uint8_t lift);   // sterge doar setul liftului indicat
    bool commCountIsEnabled(uint8_t lift);

    // Forward cadre brute RX catre Service Box pe Serial USB
    void commAnalyzeEnable();
    void commAnalyzeDisable();
}

#endif // PROTOCOL_H
