// src/Core/Services/Protocol.cpp (UART Nativ + CRC-8-CCITT)

#include "Protocol.h"
#include "Pins.h"
#include "Config.h"
#include "Arduino.h"
#include <SoftwareSerial.h>

namespace Protocol
{
    static SoftwareSerial Serial3(Pins::RS485::CALL_RX, Pins::RS485::CALL_TX);

    // -------------------------------------------------------------------------
    // Constante de format
    // -------------------------------------------------------------------------
    static constexpr char CRC_SEPARATOR = '*';
    static constexpr uint8_t RX_FIELDS = 5;
    static constexpr uint8_t FRAME_BUFFER_SIZE = 40;

    struct RxBuffer {
        char receivedChars[FRAME_BUFFER_SIZE];
        byte ndx;
        boolean recvInProgress;
        boolean newData;
        unsigned long lastValidPacketMillis;
    };

    static RxBuffer rxLift1;
    static RxBuffer rxLift2;

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

    static void byteToHexUpper(uint8_t value, char out[3]) {
        static constexpr char HEX_CHARS[] = "0123456789ABCDEF";
        out[0] = HEX_CHARS[value >> 4];
        out[1] = HEX_CHARS[value & 0x0F];
        out[2] = '\0';
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
    // Etapa 1: extrage cadru <payload*CC> in buffer; returneaza true daca s-a
    // receptionat un cadru complet si structural valid.
    // Regula de resync: un START primit in timpul unui frame abandoneaza
    // frame-ul curent si incepe unul nou.
    // -------------------------------------------------------------------------
    static bool extrageFrame(RxBuffer &buffer, char payload[FRAME_BUFFER_SIZE]) {
        if (!buffer.newData) return false;
        buffer.newData = false;

        // Cautam separatorul CRC si end marker
        char *sep = strchr(buffer.receivedChars, CRC_SEPARATOR);
        if (sep == nullptr) return false;

        *sep = '\0';
        char *crcStr = sep + 1;

        // CRC trebuie sa fie exact 2 caractere hex uppercase, urmat de end marker
        if (crcStr[0] == '\0' || crcStr[1] == '\0' || crcStr[2] != '\0') return false;
        if (!isxdigit(static_cast<unsigned char>(crcStr[0])) ||
            !isxdigit(static_cast<unsigned char>(crcStr[1]))) return false;

        uint8_t receivedCrc;
        if (!hexByteFromChars(crcStr[0], crcStr[1], receivedCrc)) return false;

        if (crc8_ccitt(buffer.receivedChars) != receivedCrc) return false;

        strncpy(payload, buffer.receivedChars, FRAME_BUFFER_SIZE - 1);
        payload[FRAME_BUFFER_SIZE - 1] = '\0';
        return true;
    }

    // -------------------------------------------------------------------------
    // Etapa 2: parseaza payload de forma a,b,c,d,e si valideaza semantic.
    // -------------------------------------------------------------------------
    bool parseazaPachet(const char* payload, LiftState &rezultat) {
        // Numaram câmpurile
        uint8_t commaCount = 0;
        for (const char *p = payload; *p; p++) {
            if (*p == ',') commaCount++;
        }
        if (commaCount != RX_FIELDS - 1) return false;

        char bufferLocal[FRAME_BUFFER_SIZE];
        strncpy(bufferLocal, payload, sizeof(bufferLocal) - 1);
        bufferLocal[sizeof(bufferLocal) - 1] = '\0';

        // Parsam cele 5 campuri
        char *fields[RX_FIELDS];
        fields[0] = strtok(bufferLocal, ",");
        for (uint8_t i = 1; i < RX_FIELDS; i++) {
            fields[i] = strtok(nullptr, ",");
            if (fields[i] == nullptr) return false;
        }
        // Nu trebuie sa existe caractere suplimentare dupa ultimul camp;
        // strtok intoarce null la final, dar daca payload contine "a,b,c,d,e,f"
        // comascount ar fi detectat deja 5 virgule.

        // Validare numerica pentru toate campurile
        for (uint8_t i = 0; i < RX_FIELDS; i++) {
            if (!esteNumeric(fields[i])) return false;
        }

        int rawPos = atoi(fields[0]);
        if (rawPos < 0 || rawPos >= Config::Hardware::FLOORS) return false;
        rezultat.pos = static_cast<uint8_t>(rawPos);

        int rawEtd = atoi(fields[1]);
        if (rawEtd < 0 || rawEtd >= Config::Hardware::FLOORS) return false;
        rezultat.etd = static_cast<uint8_t>(rawEtd);

        int rawOcp = atoi(fields[2]);
        if (rawOcp != 0 && rawOcp != 1) return false;
        rezultat.ocp = rawOcp == 1 ? Occupancy::Busy : Occupancy::Free;

        int rawSj = atoi(fields[3]);
        if (rawSj < 0 || rawSj > 2) return false;
        rezultat.sj = rawSj == 1 ? Direction::Up : rawSj == 2 ? Direction::Down : Direction::Idle;

        int rawSvc = atoi(fields[4]);
        if (rawSvc != 1 && rawSvc != 2 && rawSvc != 3 && rawSvc != 5) return false;
        if (rawSvc == 1) rezultat.svc = ServiceState::Fault;
        else if (rawSvc == 2) rezultat.svc = ServiceState::Revision;
        else if (rawSvc == 3) rezultat.svc = ServiceState::Missing;
        else rezultat.svc = ServiceState::Normal;

        return true;
    }

    // -------------------------------------------------------------------------
    // Receptor UART cu resync pe START in timpul receptiei.
    // -------------------------------------------------------------------------
    static void eantioneazaUART(HardwareSerial &serial, RxBuffer &buffer) {
        while (serial.available() > 0 && !buffer.newData) {
            char rc = serial.read();

            if (rc == Config::Protocol::START_MARKER) {
                // Resync: orice START reseteaza receptorul
                buffer.recvInProgress = true;
                buffer.ndx = 0;
                continue;
            }

            if (!buffer.recvInProgress) {
                continue;
            }

            if (rc == Config::Protocol::END_MARKER) {
                buffer.receivedChars[buffer.ndx] = '\0';
                buffer.recvInProgress = false;
                buffer.ndx = 0;
                buffer.newData = true;
                continue;
            }

            if (buffer.ndx < FRAME_BUFFER_SIZE - 1) {
                buffer.receivedChars[buffer.ndx++] = rc;
            }
            // daca bufferul e plin, continuam sa rescriem ultimul caracter
        }
    }

    // -------------------------------------------------------------------------
    // Initializare
    // -------------------------------------------------------------------------
    void init() {
        Serial1.setRX(Pins::RS485::LIFT2_RX);
        Serial2.setRX(Pins::RS485::LIFT1_RX);
        Serial1.begin(Config::Protocol::SERIAL_BAUD);
        Serial2.begin(Config::Protocol::SERIAL_BAUD);
        Serial3.begin(Config::Protocol::SERIAL_BAUD);
        pinMode(Pins::RS485::TX_ENABLE, OUTPUT);
        digitalWrite(Pins::RS485::TX_ENABLE, LOW);
        memset(&rxLift1, 0, sizeof(RxBuffer));
        memset(&rxLift2, 0, sizeof(RxBuffer));
        rxLift1.lastValidPacketMillis = millis();
        rxLift2.lastValidPacketMillis = millis();
    }

    // -------------------------------------------------------------------------
    // Update principal
    // -------------------------------------------------------------------------
    void update(SharedPanel &localPanel) {
        char payload[FRAME_BUFFER_SIZE];

        eantioneazaUART(Serial2, rxLift1);
        if (extrageFrame(rxLift1, payload)) {
            LiftState tempState;
            if (parseazaPachet(payload, tempState)) {
                localPanel.lift1 = tempState;
                rxLift1.lastValidPacketMillis = millis();
            }
            // orice invalid => discard fara contorizare
        }
        if (millis() - rxLift1.lastValidPacketMillis > Config::Protocol::PACKET_TIMEOUT_MS) {
            localPanel.lift1.svc = ServiceState::Missing;
        }

        eantioneazaUART(Serial1, rxLift2);
        if (extrageFrame(rxLift2, payload)) {
            LiftState tempState;
            if (parseazaPachet(payload, tempState)) {
                localPanel.lift2 = tempState;
                rxLift2.lastValidPacketMillis = millis();
            }
            // orice invalid => discard fara contorizare
        }
        if (millis() - rxLift2.lastValidPacketMillis > Config::Protocol::PACKET_TIMEOUT_MS) {
            localPanel.lift2.svc = ServiceState::Missing;
        }
    }

    // -------------------------------------------------------------------------
    // Transmisie apel cu acelasi format <payload*CC>
    // -------------------------------------------------------------------------
    void trimiteApel(uint8_t ascAlocat) {
        if (ascAlocat == 0) return;

        char payload[16];
        snprintf(payload, sizeof(payload), "%u,%u", ascAlocat, Config::Hardware::PANEL_FLOOR);
        uint8_t crc = crc8_ccitt(payload);
        char crcStr[3];
        byteToHexUpper(crc, crcStr);

        Serial3.listen();
        delayMicroseconds(Config::Protocol::RS485_LISTEN_SETTLE_US);
        digitalWrite(Pins::RS485::TX_ENABLE, HIGH);
        delayMicroseconds(Config::Protocol::RS485_TX_SETTLE_US);

        Serial3.print(Config::Protocol::START_MARKER);
        Serial3.print(payload);
        Serial3.print(CRC_SEPARATOR);
        Serial3.print(crcStr);
        Serial3.print(Config::Protocol::END_MARKER);

        Serial3.flush();
        delayMicroseconds(Config::Protocol::RS485_TX_RELEASE_US);
        digitalWrite(Pins::RS485::TX_ENABLE, LOW);
    }
}
