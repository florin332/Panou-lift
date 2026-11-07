// Encoder service: transport-independent framing and payload handling.

#include "Protocol.h"
#include "EncoderProtocol.h"
#include "FrameDecoder.h"
#include "Pins.h"
#include "Config.h"
#include "Transport.h"
#include "Arduino.h"
#include <atomic>

namespace Protocol
{
    // -------------------------------------------------------------------------
    // Constante de format
    // -------------------------------------------------------------------------
    static constexpr uint8_t FRAME_BUFFER_SIZE = 40;

    struct RxBuffer {
        char receivedChars[FRAME_BUFFER_SIZE];
        unsigned long lastValidPacketMillis;
        FrameDecoder decoder;

        RxBuffer() : decoder(receivedChars, sizeof(receivedChars)) {}
        void reset() {
            lastValidPacketMillis = 0;
            decoder.reset();
        }
    };

    static RxBuffer rxLift1;

    // -------------------------------------------------------------------------
    // Contoare receptie detaliate: inactive dupa reboot, pornite din Service Box
    // Singurul canal RX activ: ascensorul cabinei.
    // -------------------------------------------------------------------------
    static std::atomic<bool> sCountingEnabled{false};
    static std::atomic<bool> sResetCountersRequested{false};
    static uint32_t sRxFrames[1]      = {0}; // cadre complet delimitate (START...END)
    static uint32_t sRxValid[1]       = {0}; // cadre acceptate (format+CRC+domeniu)
    static uint32_t sRxTimeout[1]     = {0}; // cadru inceput, neterminat in timeout
    static uint32_t sRxFormatError[1] = {0}; // delimitatori/campuri/format invalide
    static uint32_t sRxCrcError[1]    = {0}; // CRC prezent, dar incorect
    static uint32_t sRxDataError[1]   = {0}; // format corect, valori imposibile

    static void resetCounters() {
        for (uint8_t i = 0; i < 1; i++) {
            sRxFrames[i] = sRxValid[i] = sRxTimeout[i] = 0;
            sRxFormatError[i] = sRxCrcError[i] = sRxDataError[i] = 0;
        }
    }

    // Rezultatul extragerii unui cadru complet delimitat
    // -------------------------------------------------------------------------
    // Etapa 1: extrage cadru <payload*CC> in buffer.
    // Regula de resync: un START primit in timpul unui frame abandoneaza
    // frame-ul curent si incepe unul nou.
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // Receptor UART cu resync pe START in timpul receptiei.
    // liftIdx ramane parametrul indexat al contoarelor RX.
    // -------------------------------------------------------------------------
    static void processByte(RxBuffer &buffer, char value, uint8_t liftIdx) {
        const FrameDecoder::Event event = buffer.decoder.feed(
            value, Config::Protocol::START_MARKER, Config::Protocol::END_MARKER, millis());

        if (event == FrameDecoder::Event::Resynchronized) {
            if (sCountingEnabled.load(std::memory_order_relaxed)) sRxFormatError[liftIdx]++;
        }

        if (event == FrameDecoder::Event::Complete) {
            if (sCountingEnabled.load(std::memory_order_relaxed)) sRxFrames[liftIdx]++;
        }
    }

    static void verifyFrameTimeout(RxBuffer &buffer, uint8_t liftIdx) {
        if (buffer.decoder.checkTimeout(millis(), Config::Protocol::PACKET_TIMEOUT_MS)) {
            if (sCountingEnabled.load(std::memory_order_relaxed)) sRxTimeout[liftIdx]++;
        }
    }

    // -------------------------------------------------------------------------
    // Initializare
    // -------------------------------------------------------------------------
    void init() {
        pinMode(Pins::Inputs::ALARM, INPUT_PULLUP);
        pinMode(Pins::Inputs::OVERLOAD, INPUT_PULLUP);
        rxLift1.reset();
        rxLift1.lastValidPacketMillis = millis();
        // Dupa reboot numararea este inactiva si contoarele sunt zero
        sCountingEnabled.store(false, std::memory_order_relaxed);
        sResetCountersRequested.store(false, std::memory_order_relaxed);
        resetCounters();
    }

    // -------------------------------------------------------------------------
    // Update principal
    // -------------------------------------------------------------------------
    void update(SharedPanel &localPanel) {
        char payload[FRAME_BUFFER_SIZE];

        if (sResetCountersRequested.exchange(false, std::memory_order_relaxed)) {
            resetCounters();
        }

        Transport::IByteTransport &transport = Transport::instance();
        while (transport.available(Transport::Channel::EncoderRx) > 0 && !rxLift1.decoder.hasFrame()) {
            processByte(rxLift1, static_cast<char>(transport.read(Transport::Channel::EncoderRx)), 0);
        }
        verifyFrameTimeout(rxLift1, 0);

        EncoderProtocol::FrameResult frame1 = EncoderProtocol::FrameResult::None;
        if (rxLift1.decoder.hasFrame()) {
            frame1 = rxLift1.decoder.overflowed()
                ? EncoderProtocol::FrameResult::FormatError
                : EncoderProtocol::validateFrame(rxLift1.decoder.frame(), payload, FRAME_BUFFER_SIZE);
            rxLift1.decoder.consumeFrame();
        }
        if (frame1 == EncoderProtocol::FrameResult::Valid) {
            LiftState tempState;
            EncoderProtocol::ParseResult parsed = EncoderProtocol::parsePayload(payload, Config::Hardware::FLOORS, tempState);
            if (parsed == EncoderProtocol::ParseResult::Valid) {
                localPanel.lift1 = tempState;
                rxLift1.lastValidPacketMillis = millis();
                if (sCountingEnabled.load(std::memory_order_relaxed)) sRxValid[0]++;
            } else if (sCountingEnabled.load(std::memory_order_relaxed)) {
                if (parsed == EncoderProtocol::ParseResult::FormatError) sRxFormatError[0]++;
                else sRxDataError[0]++;
            }
        } else if (frame1 == EncoderProtocol::FrameResult::CrcError) {
            if (sCountingEnabled.load(std::memory_order_relaxed)) sRxCrcError[0]++;
        } else if (frame1 == EncoderProtocol::FrameResult::FormatError) {
            if (sCountingEnabled.load(std::memory_order_relaxed)) sRxFormatError[0]++;
        }
        if (millis() - rxLift1.lastValidPacketMillis > Config::Protocol::PACKET_TIMEOUT_MS) {
            localPanel.lift1.svc = ServiceState::Missing;
        }

        // Exportam contoarele catre Core 0 prin memoria partajata
        localPanel.comm.lift1.rxFrames      = sRxFrames[0];
        localPanel.comm.lift1.rxValid       = sRxValid[0];
        localPanel.comm.lift1.rxTimeout     = sRxTimeout[0];
        localPanel.comm.lift1.rxFormatError = sRxFormatError[0];
        localPanel.comm.lift1.rxCrcError    = sRxCrcError[0];
        localPanel.comm.lift1.rxDataError   = sRxDataError[0];
        localPanel.comm.countingEnabled     = sCountingEnabled.load(std::memory_order_relaxed) ? 1 : 0;
    }

    // -------------------------------------------------------------------------
    // Cererile de control pot veni de pe Core 0; contoarele se modifica doar pe Core 1.
    // -------------------------------------------------------------------------
    void commCountEnable() {
        sCountingEnabled.store(true, std::memory_order_relaxed);
    }

    void commCountDisable() {
        sCountingEnabled.store(false, std::memory_order_relaxed);
    }

    void commCountReset() {
        sResetCountersRequested.store(true, std::memory_order_relaxed);
    }

    bool commCountIsEnabled() {
        return sCountingEnabled.load(std::memory_order_relaxed);
    }

}
