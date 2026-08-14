// Drivers/Protocol.cpp (UART Nativ si Transmisie Fixa - V10.29)

#include "Protocol.h"
#include "Pins.h"
#include "Config.h"
#include "Arduino.h"
#include <SoftwareSerial.h>

namespace Protocol
{
    static SoftwareSerial Serial3(Pins::RS485::CALL_RX, Pins::RS485::CALL_TX);

    struct RxBuffer {
        char receivedChars[32];
        char tempChars[32];
        byte ndx;
        boolean recvInProgress;
        boolean newData;
        unsigned long lastValidPacketMillis;
    };

    static RxBuffer rxLift1;
    static RxBuffer rxLift2;

    static bool esteNumeric(const char *str) {
        if (str == NULL || *str == '\0') return false;
        if (*str == '-') str++;
        while (*str) {
            if (!isdigit((unsigned char)*str)) return false;
            str++;
        }
        return true;
    }

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

    bool parseazaPachet(const char* sirBrut, LiftState &rezultat) {
        uint8_t commaCount = 0;
        for (const char *p = sirBrut; *p; p++) {
            if (*p == ',') commaCount++;
        }
        if (commaCount != 4) return false;

        char bufferLocal[32];
        strncpy(bufferLocal, sirBrut, sizeof(bufferLocal) - 1);
        bufferLocal[sizeof(bufferLocal) - 1] = '\0';

        char *token = strtok(bufferLocal, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawPos = atoi(token);
        if (rawPos < 0 || rawPos >= Config::Hardware::FLOORS) return false;
        rezultat.pos = static_cast<uint8_t>(rawPos);

        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawEtd = atoi(token);
        if (rawEtd < 0 || rawEtd >= Config::Hardware::FLOORS) return false;
        rezultat.etd = static_cast<uint8_t>(rawEtd);

        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawOcp = atoi(token);
        if (rawOcp != 0 && rawOcp != 1) return false;
        rezultat.ocp = rawOcp == 1 ? Occupancy::Busy : Occupancy::Free;

        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawSj = atoi(token);
        if (rawSj < 0 || rawSj > 2) return false;
        rezultat.sj = rawSj == 1 ? Direction::Up : rawSj == 2 ? Direction::Down : Direction::Idle;

        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawSvc = atoi(token);
        if (rawSvc != 1 && rawSvc != 2 && rawSvc != 3 && rawSvc != 5) return false;
        if (rawSvc == 1) rezultat.svc = ServiceState::Fault;
        else if (rawSvc == 2) rezultat.svc = ServiceState::Revision;
        else if (rawSvc == 3) rezultat.svc = ServiceState::Missing;
        else rezultat.svc = ServiceState::Normal;
        return true;
    }

    static void eantioneazaUART(HardwareSerial &serial, RxBuffer &buffer) {
        while (serial.available() > 0 && !buffer.newData) {
            char rc = serial.read();
            if (buffer.recvInProgress) {
                if (rc != Config::Protocol::END_MARKER) {
                    buffer.receivedChars[buffer.ndx++] = rc;
                    if (buffer.ndx >= 32) buffer.ndx = 31;
                } else {
                    buffer.receivedChars[buffer.ndx] = '\0';
                    buffer.recvInProgress = false;
                    buffer.ndx = 0;
                    buffer.newData = true;
                }
            } else if (rc == Config::Protocol::START_MARKER) {
                buffer.recvInProgress = true;
            }
        }
    }

    void update(SharedPanel &localPanel) {
        eantioneazaUART(Serial2, rxLift1);
        if (rxLift1.newData) {
            LiftState tempState;
            if (parseazaPachet(rxLift1.receivedChars, tempState)) {
                localPanel.lift1 = tempState;
                rxLift1.lastValidPacketMillis = millis();
            } else localPanel.system.packetErrorsLift1++;
            rxLift1.newData = false;
        }
        if (millis() - rxLift1.lastValidPacketMillis > Config::Protocol::PACKET_TIMEOUT_MS)
            localPanel.lift1.svc = ServiceState::Missing;

        eantioneazaUART(Serial1, rxLift2);
        if (rxLift2.newData) {
            LiftState tempState;
            if (parseazaPachet(rxLift2.receivedChars, tempState)) {
                localPanel.lift2 = tempState;
                rxLift2.lastValidPacketMillis = millis();
            } else localPanel.system.packetErrorsLift2++;
            rxLift2.newData = false;
        }
        if (millis() - rxLift2.lastValidPacketMillis > Config::Protocol::PACKET_TIMEOUT_MS)
            localPanel.lift2.svc = ServiceState::Missing;
    }

    void trimiteApel(uint8_t ascAlocat) {
        if (ascAlocat == 0) return;
        Serial3.listen();
        delayMicroseconds(Config::Protocol::RS485_LISTEN_SETTLE_US);
        digitalWrite(Pins::RS485::TX_ENABLE, HIGH);
        delayMicroseconds(Config::Protocol::RS485_TX_SETTLE_US);
        Serial3.print(Config::Protocol::START_MARKER);
        Serial3.print(ascAlocat);
        Serial3.print(",");
        Serial3.print(Config::Hardware::PANEL_FLOOR);
        Serial3.print(Config::Protocol::END_MARKER);
        Serial3.println();
        Serial3.flush();
        delayMicroseconds(Config::Protocol::RS485_TX_RELEASE_US);
        digitalWrite(Pins::RS485::TX_ENABLE, LOW);
    }
}
