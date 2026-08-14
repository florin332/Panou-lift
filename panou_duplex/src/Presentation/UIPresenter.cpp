#include "UIPresenter.h"
#include "Arduino.h"

namespace UIPresenter
{
    void init() {
        // Implementare vidă: independență totală de platforma grafică
    }

    void buildPage(const ProcessedDiagnostics &info, DiagnosticsProfile profile, uint8_t pageIndex, PageModel &outputModel) {
        // Delegăm construcția structurală pură către mașina de stări DiagnosticsPages.
        // UIPresenter decide fluxul pe profile de autorizare, izolând logica de motorul de desenare.
        if (profile == DiagnosticsProfile::Service) {
            DiagnosticsPages::ServicePage sPage = static_cast<DiagnosticsPages::ServicePage>(pageIndex % DiagnosticsPages::TOTAL_SERVICE_PAGES);
            DiagnosticsPages::buildServicePage(info, sPage, outputModel);
        } else {
            DiagnosticsPages::DevPage dPage = static_cast<DiagnosticsPages::DevPage>(pageIndex % DiagnosticsPages::TOTAL_DEV_PAGES);
            DiagnosticsPages::buildDeveloperPage(info, dPage, outputModel);
        }
    }
}
