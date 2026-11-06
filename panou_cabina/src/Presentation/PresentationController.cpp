#include "PresentationController.h"

#include "Arduino.h"
#include "SharedPanel.h"
#include "DiagnosticsNavigator.h"
#include "UIPresenter.h"
#include "DisplayRenderer.h"
#include "PanelRenderer.h"
#include "ServiceMenu.h"
#include "DiagnosticService.h"
#include "ServiceProtocol.h"
#include "Presentation.h"
#include "Pins.h"
#include "Display.h"

namespace PresentationController
{
    void init() {
        Presentation::init();
        ServiceProtocol::init();
        ServiceMenu::init();
    }

    void update() {
        ServiceProtocol::poll();

        static bool overloadImageWasActive = false;
        if (digitalRead(Pins::Inputs::OVERLOAD) == LOW) {
            Display::showOverloadImage();
            overloadImageWasActive = true;
            return;
        }

        if (overloadImageWasActive) {
            Display::hideOverloadImage();
            PanelRenderer::invalidate(DisplayTarget::Panel);
            overloadImageWasActive = false;
        }

        const bool wasInServiceMode = ServiceMenu::isInServiceMode();
        ServiceMenu::update();
        const bool enteredService = !wasInServiceMode && ServiceMenu::isInServiceMode();
        const bool returnedFromService = wasInServiceMode && !ServiceMenu::isInServiceMode();
        if (enteredService || returnedFromService) {
            PanelRenderer::invalidate(DisplayTarget::Panel);
        }

        if (ServiceMenu::isInServiceMode()) return;

        SharedPanel snapshot = {};
        if (!ApplicationState::read(snapshot)) return;

        const bool displayWoke = Presentation::update(snapshot);
        if (displayWoke) PanelRenderer::invalidate(DisplayTarget::Panel);

        if (snapshot.lift1.ocp == Occupancy::Free && snapshot.lift1.svc == ServiceState::Normal) {
            return;
        }

        const DiagnosticsNavigator::NavigatorState &nav = DiagnosticsNavigator::getState();
        if (!nav.isMenuOpen) {
            PanelRenderer::render(DisplayTarget::Panel, snapshot.lift1, "ASCENSOR");
        } else {
            PanelRenderer::invalidate(DisplayTarget::Panel);
        }

        static unsigned long lastDiagMillis = 0;
        if (nav.isMenuOpen && millis() - lastDiagMillis >= 500) {
            lastDiagMillis = millis();
            ProcessedDiagnostics info;
            DiagnosticService::interpret(snapshot, info);
            PageModel logicalPage;
            const uint8_t pageIdx = (nav.currentProfile == DiagnosticsProfile::Service)
                ? nav.servicePageIndex : nav.developerPageIndex;
            UIPresenter::buildPage(info, nav.currentProfile, pageIdx, logicalPage);
            DisplayRenderer::render(DisplayTarget::Panel, logicalPage);
        }

        if (displayWoke) Display::showBacklight();
    }
}