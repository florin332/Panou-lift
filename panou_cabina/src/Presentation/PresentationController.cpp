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
#include "Transport.h"
#include <cstring>

namespace PresentationController
{
    namespace {
        struct ServiceInputBuffer {
            char data[64];
            uint8_t length = 0;
        };

        ServiceInputBuffer sUsbInput;
        ServiceInputBuffer sEthernetInput;

        void sendServiceResponse(const char *response) {
            Serial.println(response);
            Transport::IByteTransport &transport = Transport::instance();
            transport.write(Transport::Channel::Service,
                            reinterpret_cast<const uint8_t *>(response), strlen(response));
            const uint8_t lineEnding[] = {'\r', '\n'};
            transport.write(Transport::Channel::Service, lineEnding, sizeof(lineEnding));
        }

        void consumeServiceByte(ServiceInputBuffer &input, char value) {
            if (value == '\n' || value == '\r') {
                if (input.length > 0) {
                    input.data[input.length] = '\0';
                    ServiceProtocol::submitLine(input.data);
                    input.length = 0;
                }
            } else if (input.length < sizeof(input.data) - 1) {
                input.data[input.length++] = value;
            }
        }

        void pollServiceInputs() {
            while (Serial.available()) {
                consumeServiceByte(sUsbInput, static_cast<char>(Serial.read()));
            }

            Transport::IByteTransport &transport = Transport::instance();
            while (transport.available(Transport::Channel::Service) > 0) {
                const int value = transport.read(Transport::Channel::Service);
                if (value < 0) break;
                consumeServiceByte(sEthernetInput, static_cast<char>(value));
            }
        }
    }

    void init() {
        Presentation::init();
        Transport::init();
        ServiceProtocol::init();
        ServiceProtocol::setResponseSink(sendServiceResponse);
        ServiceMenu::init();
    }

    void update() {
        Transport::poll();
        pollServiceInputs();

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