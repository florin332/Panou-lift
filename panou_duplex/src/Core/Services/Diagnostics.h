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
    uint16_t boots;
    const char* resetReason; 
    uint32_t uptime;

    // Gruparea simetrică a cabinelor
    LiftDiagnostics lift1;
    LiftDiagnostics lift2;

    // Statistici de sistem
    uint16_t errorsLift1;
    uint16_t errorsLift2;
    uint16_t seqlockCollisions;
};

namespace Diagnostics
{
    void init();

    // INTERPRETORUL IERARHIZAT (Pasul 1): Traduce stările raw direct în structura compusă
    void interpret(const SharedPanel &snapshot, ProcessedDiagnostics &output);
}

#endif // DIAGNOSTICS_H
