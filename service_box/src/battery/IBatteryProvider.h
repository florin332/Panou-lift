#ifndef I_BATTERY_PROVIDER_H
#define I_BATTERY_PROVIDER_H

#include <Arduino.h>

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
};

#endif // I_BATTERY_PROVIDER_H
