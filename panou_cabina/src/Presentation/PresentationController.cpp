#include "PresentationController.h"

#include "Arduino.h"
#include "SharedPanel.h"
#include "DisplayRenderer.h"
#include "PanelRenderer.h"
#include "Graphics.h"
#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "Presentation.h"
#include "Pins.h"
#include "Transport.h"
#include "WebUi.h"
#include <atomic>
#include <cstring>
#include <pico/multicore.h>

namespace PresentationController
{
    namespace {
        struct ServiceInputBuffer {
            char data[64];
            uint8_t length = 0;
        };

        ServiceInputBuffer sUsbInput;
        ServiceInputBuffer sEthernetInput;
        constexpr uint8_t RESPONSE_QUEUE_SIZE = 8;
        constexpr size_t RESPONSE_SIZE = 96;
        char sResponseQueue[RESPONSE_QUEUE_SIZE][RESPONSE_SIZE] = {};
        std::atomic<uint8_t> sResponseHead{0};
        std::atomic<uint8_t> sResponseTail{0};
        std::atomic<bool> sCore1Ready{false};

        void sendServiceResponse(const char *response) {
            WebUi::captureServiceResponse(response);
            Serial.println(response);
            Transport::IByteTransport &transport = Transport::instance();
            transport.write(Transport::Channel::Service,
                            reinterpret_cast<const uint8_t *>(response), strlen(response));
            const uint8_t lineEnding[] = {'\r', '\n'};
            transport.write(Transport::Channel::Service, lineEnding, sizeof(lineEnding));
        }

        void queueOrSendServiceResponse(const char *response) {
            if (get_core_num() == 0) {
                sendServiceResponse(response);
                return;
            }

            const uint8_t head = sResponseHead.load(std::memory_order_relaxed);
            const uint8_t next = static_cast<uint8_t>((head + 1) % RESPONSE_QUEUE_SIZE);
            if (next == sResponseTail.load(std::memory_order_acquire)) return;

            strncpy(sResponseQueue[head], response, RESPONSE_SIZE - 1);
            sResponseQueue[head][RESPONSE_SIZE - 1] = '\0';
            sResponseHead.store(next, std::memory_order_release);
        }

        void flushServiceResponses() {
            uint8_t tail = sResponseTail.load(std::memory_order_relaxed);
            const uint8_t head = sResponseHead.load(std::memory_order_acquire);
            while (tail != head) {
                sendServiceResponse(sResponseQueue[tail]);
                tail = static_cast<uint8_t>((tail + 1) % RESPONSE_QUEUE_SIZE);
                sResponseTail.store(tail, std::memory_order_release);
            }
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

    void initCore0() {
        sCore1Ready.store(false, std::memory_order_relaxed);
        Transport::init();
        ServiceProtocol::init();
        ServiceProtocol::setResponseSink(queueOrSendServiceResponse);
    }

    void initCore1() {
        ServiceMenu::init();
        Presentation::init();
        sCore1Ready.store(true, std::memory_order_release);
    }

    void pollCore0() {
        if (!sCore1Ready.load(std::memory_order_acquire)) return;
        Transport::poll();
        pollServiceInputs();
        flushServiceResponses();
    }

    void updateCore1() {
        if (!sCore1Ready.load(std::memory_order_acquire)) return;
        static bool overloadImageWasActive = false;
        if (digitalRead(Pins::Inputs::OVERLOAD) == LOW) {
            DisplayRenderer::showOverloadImage();
            overloadImageWasActive = true;
            return;
        }

        if (overloadImageWasActive) {
            DisplayRenderer::hideOverloadImage();
            PanelRenderer::invalidate(GraphicsTarget::Panel);
            overloadImageWasActive = false;
        }

        const bool wasInServiceMode = ServiceMenu::isInServiceMode();
        ServiceMenu::update();
        const bool enteredService = !wasInServiceMode && ServiceMenu::isInServiceMode();
        const bool returnedFromService = wasInServiceMode && !ServiceMenu::isInServiceMode();
        if (enteredService || returnedFromService) {
            PanelRenderer::invalidate(GraphicsTarget::Panel);
        }

        if (ServiceMenu::isInServiceMode()) return;

        SharedPanel snapshot = {};
        if (!ApplicationState::read(snapshot)) return;

        const bool displayWoke = Presentation::update(snapshot);
        if (displayWoke) PanelRenderer::invalidate(GraphicsTarget::Panel);

        if (snapshot.lift1.ocp == Occupancy::Free && snapshot.lift1.svc == ServiceState::Normal) {
            return;
        }

        PanelRenderer::render(GraphicsTarget::Panel, snapshot.lift1, "ASCENSOR");

        if (displayWoke) DisplayRenderer::showBacklight();
    }
}