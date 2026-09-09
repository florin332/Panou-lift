#ifndef BATTERY_STUB_H
#define BATTERY_STUB_H

#include <Arduino.h>
#include "IBatteryProvider.h"

// ============================================================================
// STUB / PROVIZORIU pentru testarea paginii Battery Check pe branch-ul
// graphic_ui. Permite simularea nivelului bateriei și a stării de încărcare
// prin comenzi seriale.
//
// Starea stub-ului (nivel și încărcare) este persistată în EEPROM pentru a
// supraviețui repornirilor. Acest lucru permite testarea scenariilor LOW,
// MEDIUM, GOOD, CHARGING fără a retrimite comanda la fiecare boot.
//
// Acest fișier este DESTINAT ELIMINĂRII la integrarea implementării reale de
// pe branch-urile marble/waveshare. Eliminarea constă doar în:
//   1. Ștergerea includerii #include "battery/BatteryStub.h"
//   2. Înlocuirea instanței BatteryStub cu implementarea reală în main.cpp
//   3. Ștergerea apelurilor batteryStub.update() din loop().
//
// NU se modifică BatteryCheckScreen sau IBatteryProvider.
// ============================================================================

class BatteryStub : public IBatteryProvider {
public:
    // Stare inițială: MEDIUM (40%), fără încărcare.
    // NU accesează EEPROM; pentru inițializarea hardware se apelează begin().
    BatteryStub();

    // Inițializează EEPROM și încarcă starea persistată.
    // Trebuie apelat din setup(), după inițializarea hardware-ului.
    void begin();

    // Procesează comenzi seriale pentru testare.
    // Apelat în fiecare iterație loop().
    void update();

    // IBatteryProvider
    uint8_t getLevelPercent() override;
    bool isCharging() override;

    // Setări explicite, utile pentru teste programmatic sau la inițializare.
    // După fiecare setare, starea este salvată în EEPROM.
    void setLevel(uint8_t level);
    void setCharging(bool charging);

private:
    uint8_t _level;      // 0 .. 100
    bool    _charging;

    // Persistență EEPROM
    // Adresa de start este aleasă pentru a nu se suprapune cu datele de
    // calibrare touch care folosesc offsetul 0 (Waveshare 32 bytes, Marble
    // 64 bytes). Stub-ul folosește 16 bytes începând de la offsetul 512.
    static constexpr uint16_t STUB_EEPROM_OFFSET = 512;
    static constexpr uint16_t EEPROM_SIZE = (STUB_EEPROM_OFFSET + 16);
    static constexpr uint32_t STUB_MAGIC  = 0x42535442;  // "BSTB"
    static constexpr uint16_t STUB_VERSION = 1;

    struct __attribute__((packed)) StubState {
        uint32_t magic;
        uint16_t version;
        uint8_t  level;
        uint8_t  charging;
    };

    void loadState();
    void saveState();

    void printStatus();
    void processCommand(const String& cmd);
};

#endif // BATTERY_STUB_H
