#include "DiagnosticService.h"

namespace DiagnosticService
{
    void init() {
        Diagnostics::init();
    }

    void interpret(const SharedPanel &state, ProcessedDiagnostics &output) {
        Diagnostics::interpret(state, output);
    }
}