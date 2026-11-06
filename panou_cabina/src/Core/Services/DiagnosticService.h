#ifndef DIAGNOSTIC_SERVICE_H
#define DIAGNOSTIC_SERVICE_H

#include "Diagnostics.h"

namespace DiagnosticService
{
    void init();
    void interpret(const SharedPanel &state, ProcessedDiagnostics &output);
}

#endif