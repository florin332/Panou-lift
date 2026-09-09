#ifndef I_BATTERY_PROVIDER_H
#define I_BATTERY_PROVIDER_H

#include <Arduino.h>

// ============================================================================
// Stare bateriei determinată de Battery Manager (hardware-dependent).
// UI-ul / sistemul de navigație aplică politica în funcție de această stare.
//
//   NORMAL   - nivel suficient (de la MEDIUM la FULL); operațiuni normale.
//   LOW      - avertizare; nu se pot porni teste noi, dar cele în desfășurare
//              continuă.
//   CRITICAL - nivel critic; sistemul întrerupe controlat testele și revine
//              la Battery Check.
//   CHARGING - baterie în curs de încărcare.
// ============================================================================
enum class BatteryState {
    NORMAL,
    LOW,
    CRITICAL,
    CHARGING
};

// ============================================================================
// Interfață abstractă pentru furnizorul de stare baterie.
//
// Pagina BatteryCheckScreen primește o referință la această interfață și nu
// depinde de implementarea concretă (stub sau driver hardware real).
//
// La integrarea pe branch-urile hardware marble/waveshare, se va implementa
// o clasă reală (ex. BspBatteryProvider) care respectă această interfață și
// se va înlocui instanța stub în main.cpp, fără modificări în UI.
// ============================================================================

class IBatteryProvider {
public:
    virtual ~IBatteryProvider() = default;

    // Returnează nivelul bateriei în procente (0 .. 100).
    virtual uint8_t getLevelPercent() = 0;

    // Returnează true dacă bateria este în curs de încărcare.
    virtual bool isCharging() = 0;

    // Returnează starea discretă a bateriei (NORMAL / LOW / CRITICAL / CHARGING).
    // Stabilirea stării ține de Battery Manager (hardware-dependent); UI-ul
    // aplică doar politica asociată stării curente.
    virtual BatteryState getState() = 0;
};

#endif // I_BATTERY_PROVIDER_H
