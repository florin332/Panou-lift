// src/Core/Services/Protocol.cpp (UART Nativ + CRC-8-CCITT)

#include "Protocol.h"
#include "Pins.h"
#include "Config.h"
#include "Arduino.h"

namespace Protocol
{
    // -------------------------------------------------------------------------
    // Constante de format
    // -------------------------------------------------------------------------
    static constexpr char CRC_SEPARATOR = '*';
    static constexpr uint8_t RX_FIELDS = 5;
    static constexpr uint8_t FRAME_BUFFER_SIZE = 40;
    static constexpr uint8_t ANALYZE_BUFFER_SIZE = 96;

    struct RxBuffer {
        char receivedChars[FRAME_BUFFER_SIZE];
        byte ndx;
        boolean recvInProgress;
        boolean newData;
        unsigned long lastValidPacketMillis;
        unsigned long frameStartMillis;   // setat la START, pentru timeout cadru
        boolean frameTimedOut;            // timeout deja contorizat pentru cadrul curent
        char analyzeChars[ANALYZE_BUFFER_SIZE];
        uint8_t analyzeLength;
        boolean analyzeActive;
        boolean analyzeOverflow;
    };

    static RxBuffer rxLift1;

    // -------------------------------------------------------------------------
    // Contoare receptie detaliate: inactive dupa reboot, pornite din Service Box
    // Singurul canal RX activ: ascensorul cabinei.
    // -------------------------------------------------------------------------
    static bool sCountingEnabled = false;
    static bool sAnalyzeEnabled = false;
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

    static void appendAnalyzeByte(RxBuffer &buffer, char value) {
        if (buffer.analyzeLength < sizeof(buffer.analyzeChars)) {
            buffer.analyzeChars[buffer.analyzeLength++] = value;
        } else {
            buffer.analyzeOverflow = true;
        }
    }

    static void emitAnalyzeFrame(const RxBuffer &buffer, uint8_t liftIdx, bool complete) {
        Serial.print("RAW L");
        Serial.print(liftIdx + 1);
        Serial.print(complete ? ":" : " PARTIAL:");
        Serial.write(reinterpret_cast<const uint8_t*>(buffer.analyzeChars), buffer.analyzeLength);
        if (buffer.analyzeOverflow) Serial.print("[TRUNCATED]");
        Serial.println();
    }

    // Rezultatul extragerii unui cadru complet delimitat
    enum class FrameResult : uint8_t { None, FormatError, CrcError, Valid };

    // Rezultatul validarii semantice a payload-ului
    enum class ParseResult : uint8_t { FormatError, DataError, Valid };

    // -------------------------------------------------------------------------
    // CRC-8-CCITT: polinom 0x07, init 0x00, fara reflectare, fara XOR final
    // -------------------------------------------------------------------------
    static uint8_t crc8_ccitt(const char *data) {
        uint8_t crc = 0x00;
        while (*data) {
            crc ^= static_cast<uint8_t>(*data++);
            for (uint8_t i = 0; i < 8; ++i) {
                crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
            }
        }
        return crc;
    }

    static bool hexByteFromChars(char high, char low, uint8_t &out) {
        auto nibble = [](char c) -> int8_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        };
        int8_t h = nibble(high);
        int8_t l = nibble(low);
        if (h < 0 || l < 0) return false;
        out = static_cast<uint8_t>((h << 4) | l);
        return true;
    }

    static bool esteNumeric(const char *str) {
        if (str == nullptr || *str == '\0') return false;
        if (*str == '-') str++;
        while (*str) {
            if (!isdigit(static_cast<unsigned char>(*str))) return false;
            str++;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Etapa 1: extrage cadru <payload*CC> in buffer.
    // Regula de resync: un START primit in timpul unui frame abandoneaza
    // frame-ul curent si incepe unul nou.
    // -------------------------------------------------------------------------
    static FrameResult extrageFrame(RxBuffer &buffer, char payload[FRAME_BUFFER_SIZE]) {
        if (!buffer.newData) return FrameResult::None;
        buffer.newData = false;

        // Cautam separatorul CRC si end marker
        char *sep = strchr(buffer.receivedChars, CRC_SEPARATOR);
        if (sep == nullptr) return FrameResult::FormatError;

        *sep = '\0';
        char *crcStr = sep + 1;

        // CRC trebuie sa fie exact 2 caractere hex uppercase, urmat de end marker
        if (crcStr[0] == '\0' || crcStr[1] == '\0' || crcStr[2] != '\0') return FrameResult::FormatError;
        if (!isxdigit(static_cast<unsigned char>(crcStr[0])) ||
            !isxdigit(static_cast<unsigned char>(crcStr[1]))) return FrameResult::FormatError;

        uint8_t receivedCrc;
        if (!hexByteFromChars(crcStr[0], crcStr[1], receivedCrc)) return FrameResult::FormatError;

        if (crc8_ccitt(buffer.receivedChars) != receivedCrc) return FrameResult::CrcError;

        strncpy(payload, buffer.receivedChars, FRAME_BUFFER_SIZE - 1);
        payload[FRAME_BUFFER_SIZE - 1] = '\0';
        return FrameResult::Valid;
    }

    // -------------------------------------------------------------------------
    // Etapa 2: parseaza payload de forma a,b,c,d,e si valideaza semantic.
    // -------------------------------------------------------------------------
    static ParseResult parseazaPachet(const char* payload, LiftState &rezultat) {
        // Numaram câmpurile
        uint8_t commaCount = 0;
        for (const char *p = payload; *p; p++) {
            if (*p == ',') commaCount++;
        }
        if (commaCount != RX_FIELDS - 1) return ParseResult::FormatError;

        char bufferLocal[FRAME_BUFFER_SIZE];
        strncpy(bufferLocal, payload, sizeof(bufferLocal) - 1);
        bufferLocal[sizeof(bufferLocal) - 1] = '\0';

        // Parsam cele 5 campuri
        char *fields[RX_FIELDS];
        fields[0] = strtok(bufferLocal, ",");
        for (uint8_t i = 1; i < RX_FIELDS; i++) {
            fields[i] = strtok(nullptr, ",");
            if (fields[i] == nullptr) return ParseResult::FormatError;
        }

        // Validare numerica pentru toate campurile
        for (uint8_t i = 0; i < RX_FIELDS; i++) {
            if (!esteNumeric(fields[i])) return ParseResult::FormatError;
        }

        int rawPos = atoi(fields[0]);
        if (rawPos < 0 || rawPos >= Config::Hardware::FLOORS) return ParseResult::DataError;
        rezultat.pos = static_cast<uint8_t>(rawPos);

        int rawEtd = atoi(fields[1]);
        if (rawEtd < 0 || rawEtd >= Config::Hardware::FLOORS) return ParseResult::DataError;
        rezultat.etd = static_cast<uint8_t>(rawEtd);

        int rawOcp = atoi(fields[2]);
        if (rawOcp != 0 && rawOcp != 1) return ParseResult::DataError;
        rezultat.ocp = rawOcp == 1 ? Occupancy::Busy : Occupancy::Free;

        int rawSj = atoi(fields[3]);
        if (rawSj < 0 || rawSj > 2) return ParseResult::DataError;
        rezultat.sj = rawSj == 1 ? Direction::Up : rawSj == 2 ? Direction::Down : Direction::Idle;

        int rawSvc = atoi(fields[4]);
        if (rawSvc != 1 && rawSvc != 2 && rawSvc != 3 && rawSvc != 5) return ParseResult::DataError;
        if (rawSvc == 1) rezultat.svc = ServiceState::Fault;
        else if (rawSvc == 2) rezultat.svc = ServiceState::Revision;
        else if (rawSvc == 3) rezultat.svc = ServiceState::Missing;
        else rezultat.svc = ServiceState::Normal;

        return ParseResult::Valid;
    }

    // -------------------------------------------------------------------------
    // Receptor UART cu resync pe START in timpul receptiei.
    // liftIdx ramane parametrul indexat al contorului pentru RAW L1.
    // -------------------------------------------------------------------------
    static void eantioneazaUART(HardwareSerial &serial, RxBuffer &buffer, uint8_t liftIdx) {
        while (serial.available() > 0 && !buffer.newData) {
            char rc = serial.read();

            if (rc == Config::Protocol::START_MARKER) {
                // Resync: un START in timpul unui cadru = cadru abandonat (eroare de format)
                if (buffer.recvInProgress && !buffer.frameTimedOut && sCountingEnabled) {
                    sRxFormatError[liftIdx]++;
                }
                if (buffer.analyzeActive) {
                    emitAnalyzeFrame(buffer, liftIdx, false);
                }
                buffer.recvInProgress = true;
                buffer.frameTimedOut = false;
                buffer.frameStartMillis = millis();
                buffer.ndx = 0;
                buffer.analyzeLength = 0;
                buffer.analyzeOverflow = false;
                buffer.analyzeActive = sAnalyzeEnabled;
                if (buffer.analyzeActive) appendAnalyzeByte(buffer, rc);
                continue;
            }

            if (!buffer.recvInProgress) {
                continue;
            }

            if (rc == Config::Protocol::END_MARKER) {
                if (buffer.analyzeActive) {
                    appendAnalyzeByte(buffer, rc);
                    emitAnalyzeFrame(buffer, liftIdx, true);
                    buffer.analyzeActive = false;
                }
                buffer.receivedChars[buffer.ndx] = '\0';
                buffer.recvInProgress = false;
                buffer.ndx = 0;
                buffer.newData = true;
                // Cadru complet delimitat
                if (sCountingEnabled && !buffer.frameTimedOut) {
                    sRxFrames[liftIdx]++;
                }
                continue;
            }

            if (buffer.ndx < FRAME_BUFFER_SIZE - 1) {
                buffer.receivedChars[buffer.ndx++] = rc;
            }
            if (buffer.analyzeActive) appendAnalyzeByte(buffer, rc);
            // daca bufferul e plin, continuam sa rescriem ultimul caracter
        }
    }

    // -------------------------------------------------------------------------
    // Verifica timeout-ul unui cadru inceput (START fara END in fereastra)
    // -------------------------------------------------------------------------
    static void verificaTimeoutCadru(RxBuffer &buffer, uint8_t liftIdx) {
        if (buffer.recvInProgress && !buffer.frameTimedOut &&
            millis() - buffer.frameStartMillis > Config::Protocol::PACKET_TIMEOUT_MS) {
            buffer.frameTimedOut = true;
            buffer.recvInProgress = false;
            buffer.ndx = 0;
            if (buffer.analyzeActive) {
                emitAnalyzeFrame(buffer, liftIdx, false);
                buffer.analyzeActive = false;
            }
            if (sCountingEnabled) sRxTimeout[liftIdx]++;
        }
    }

    // -------------------------------------------------------------------------
    // Initializare
    // -------------------------------------------------------------------------
    void init() {
        Serial1.setTX(Pins::UART::ALARM_TX);
        Serial1.setRX(-1);
        Serial2.setTX(-1);
        Serial2.setRX(Pins::UART::DATA_RX);
        Serial1.begin(Config::Protocol::SERIAL_BAUD);
        Serial2.begin(Config::Protocol::SERIAL_BAUD);
        pinMode(Pins::Inputs::ALARM, INPUT);
        pinMode(Pins::Inputs::OVERLOAD, INPUT);
        memset(&rxLift1, 0, sizeof(RxBuffer));
        rxLift1.lastValidPacketMillis = millis();
        // Dupa reboot numararea este inactiva si contoarele sunt zero
        sCountingEnabled = false;
        sAnalyzeEnabled = false;
        resetCounters();
    }

    // -------------------------------------------------------------------------
    // Update principal
    // -------------------------------------------------------------------------
    void update(SharedPanel &localPanel) {
        char payload[FRAME_BUFFER_SIZE];

        eantioneazaUART(Serial2, rxLift1, 0);
        verificaTimeoutCadru(rxLift1, 0);
        FrameResult frame1 = extrageFrame(rxLift1, payload);
        if (frame1 == FrameResult::Valid) {
            LiftState tempState;
            ParseResult parsed = parseazaPachet(payload, tempState);
            if (parsed == ParseResult::Valid) {
                localPanel.lift1 = tempState;
                rxLift1.lastValidPacketMillis = millis();
                if (sCountingEnabled) sRxValid[0]++;
            } else if (sCountingEnabled) {
                if (parsed == ParseResult::FormatError) sRxFormatError[0]++;
                else sRxDataError[0]++;
            }
        } else if (frame1 == FrameResult::CrcError) {
            if (sCountingEnabled) sRxCrcError[0]++;
        } else if (frame1 == FrameResult::FormatError) {
            if (sCountingEnabled) sRxFormatError[0]++;
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
        localPanel.comm.countingEnabled     = sCountingEnabled ? 1 : 0;
    }

    // -------------------------------------------------------------------------
    // API control contoare (apelat de ServiceProtocol pe acelasi Core 1)
    // -------------------------------------------------------------------------
    void commCountEnable() {
        sCountingEnabled = true;
    }

    void commCountDisable() {
        sCountingEnabled = false;
        resetCounters();
    }

    void commCountReset() {
        resetCounters();
    }

    bool commCountIsEnabled() {
        return sCountingEnabled;
    }

    void commAnalyzeEnable() {
        sAnalyzeEnabled = true;
        rxLift1.analyzeActive = false;
    }

    void commAnalyzeDisable() {
        sAnalyzeEnabled = false;
        if (rxLift1.analyzeActive) {
            emitAnalyzeFrame(rxLift1, 0, false);
            rxLift1.analyzeActive = false;
        }
    }
}
