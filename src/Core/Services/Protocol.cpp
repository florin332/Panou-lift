// Drivers/Protocol.cpp (UART Nativ și Transmisie Fixă 

#include "Protocol.h" // Local în src/Logic/

// Rute către folderul src/ (Urcat un nivel și intrat în core)
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
        int serialCheck;
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
        rxLift1.serialCheck = 10;
        rxLift2.serialCheck = 10;
    }

    // NIVELUL: PARSER & VALIDATOR RIGID (V10.22 Securizat Aerospațial)
    bool parseazaPachet(const char* sirBrut, LiftState &rezultat) {
        // --- 1. VALIDARE STRUCTURALĂ: Număr exact de virgule despărțitoare ---
        uint8_t commaCount = 0;
        const char *p = sirBrut;
        while (*p) {
            if (*p == ',') commaCount++;
            p++;
        }
        if (commaCount != 4) return false; 

        // --- 2. COPIERE SECURIZATĂ REZISTENTĂ LA BUFFER OVERFLOW ---
        char bufferLocal[32];
        strncpy(bufferLocal, sirBrut, sizeof(bufferLocal) - 1);
        bufferLocal[sizeof(bufferLocal) - 1] = '\0'; 
        
        // --- 3. PARSARE ȘI VALIDARE STRICTĂ: POS (Etaj curent 0-17) ---
        char *token = strtok(bufferLocal, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawPos = atoi(token);
        if (rawPos < 0 || rawPos >= Config::Hardware::FLOORS) return false; 
        rezultat.pos = static_cast<uint8_t>(rawPos);

        // --- 4. PARSARE ȘI VALIDARE STRICTĂ: ETD (Etaj destinație 0-17) ---
        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawEtd = atoi(token);
        if (rawEtd < 0 || rawEtd >= Config::Hardware::FLOORS) return false; 
        rezultat.etd = static_cast<uint8_t>(rawEtd);

        // --- 5. PARSARE ȘI VALIDARE STRICTĂ: OCP (Occupancy Enum) ---
        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawOcp = atoi(token);
        if (rawOcp != 0 && rawOcp != 1) return false; 
        rezultat.ocp = (rawOcp == 1) ? Occupancy::Busy : Occupancy::Free;

        // --- 6. PARSARE ȘI VALIDARE STRICTĂ: SJ (Direction Enum) ---
        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawSj = atoi(token);
        if (rawSj < 0 || rawSj > 2) return false; 
        if (rawSj == 1)      rezultat.sj = Direction::Up;
        else if (rawSj == 2) rezultat.sj = Direction::Down;
        else                 rezultat.sj = Direction::Idle;

        // --- 7. PARSARE ȘI VALIDARE STRICTĂ: SVC (ServiceState Enum) ---
        token = strtok(NULL, ",");
        if (token == NULL || !esteNumeric(token)) return false;
        int rawSvc = atoi(token);
        if (rawSvc != 1 && rawSvc != 2 && rawSvc != 3 && rawSvc != 5) return false; 
        if (rawSvc == 1)      rezultat.svc = ServiceState::Fault;
        else if (rawSvc == 2) rezultat.svc = ServiceState::Revision;
        else if (rawSvc == 3) rezultat.svc = ServiceState::Missing;
        else                  rezultat.svc = ServiceState::Normal;

        return true; 
    }

    static void eantioneazaUART(HardwareSerial &serial, RxBuffer &buffer) {
        char rc;
        while (serial.available() > 0 && !buffer.newData) {
            rc = serial.read();
            if (buffer.recvInProgress) {
                if (rc != Config::Protocol::END_MARKER) {
                    buffer.receivedChars[buffer.ndx] = rc;
                    buffer.ndx++;
                    if (buffer.ndx >= 32) { buffer.ndx = 31; }
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
        // --- MONITORIZARE LIFT 1 ---
        if (Serial2.available() > 0) { rxLift1.serialCheck++; } else { rxLift1.serialCheck--; }
        if (rxLift1.serialCheck > 20)  { rxLift1.serialCheck = 10; }
        if (rxLift1.serialCheck < -20) { rxLift1.serialCheck = -10; }

        if (rxLift1.serialCheck > 0) {
            eantioneazaUART(Serial2, rxLift1);
            if (rxLift1.newData) {
                LiftState tempState;
                if (parseazaPachet(rxLift1.receivedChars, tempState)) {
                    localPanel.lift1 = tempState; 
                } else {
                    localPanel.system.packetErrorsLift1++; 
                }
                rxLift1.newData = false;
            }
        } else {
            localPanel.lift1.svc = ServiceState::Missing; 
        }

        // --- MONITORIZARE LIFT 2 ---
        if (Serial1.available() > 0) { rxLift2.serialCheck++; } else { rxLift2.serialCheck--; }
        if (rxLift2.serialCheck > 20)  { rxLift2.serialCheck = 10; }
        if (rxLift2.serialCheck < -20) { rxLift2.serialCheck = -10; }

        if (rxLift2.serialCheck > 0) {
            eantioneazaUART(Serial1, rxLift2);
            if (rxLift2.newData) {
                LiftState tempState;
                if (parseazaPachet(rxLift2.receivedChars, tempState)) {
                    localPanel.lift2 = tempState;
                } else {
                    localPanel.system.packetErrorsLift2++;
                }
                rxLift2.newData = false;
            }
        } else {
            localPanel.lift2.svc = ServiceState::Missing;
        }
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
