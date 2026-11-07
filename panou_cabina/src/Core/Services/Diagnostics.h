#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "SharedPanel.h"

// Sub-structura încapsulată pentru o singură cabină (Observația ta)
struct LiftDiagnostics {
    uint8_t pos;
    uint8_t etd;
    const char* sj;   // "Idle", "Up", "Down"
    const char* ocp;  // "Free", "Busy"
    const char* svc;  // "Normal", "Fault", "Revision", "No Serial"
};

// Structura generală de diagnostic ierarhizată și scalabilă (Observația ta)
struct ProcessedDiagnostics {
    const char* resetReason; 
    uint32_t uptime;
    uint32_t memoryAddress;
    uint32_t sequence;

    // Starea singurului ascensor
    LiftDiagnostics lift1;

    // Statistici de sistem
    uint32_t rxFrames;
    uint32_t rxTimeouts;
    uint32_t rxFormatErrors;
    uint32_t rxCrcErrors;
    uint32_t rxDataErrors;
};

namespace Diagnostics
{
    void init();

    // INTERPRETORUL IERARHIZAT (Pasul 1): Traduce stările raw direct în structura compusă
    void interpret(const SharedPanel &snapshot, ProcessedDiagnostics &output);
}

#endif // DIAGNOSTICS_H
