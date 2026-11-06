#ifndef DIAGNOSTICS_PAGES_H
#define DIAGNOSTICS_PAGES_H

#include "Diagnostics.h"
#include "PageModel.h"

namespace DiagnosticsPages
{
    enum class ServicePage : uint8_t { System, Lift1, Communication, Version };
    enum class DevPage : uint8_t { Engineering, MemoryLayout, Tasks, ProtocolStats, Assertions };

    constexpr uint8_t TOTAL_SERVICE_PAGES = 4;
    constexpr uint8_t TOTAL_DEV_PAGES     = 5;

    void init();
    void buildServicePage(const ProcessedDiagnostics &info, ServicePage page, PageModel &output);
    void buildDeveloperPage(const ProcessedDiagnostics &info, DevPage page, PageModel &output);
}

#endif // DIAGNOSTICS_PAGES_H
