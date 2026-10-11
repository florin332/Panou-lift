// src/Core/Services/Settings.h
// Setări persistente per-panou (etaj panou Etj, timeout standby ecran).
//
// ZONĂ PROTEJATĂ DE FLASH:
//   Folosește EEPROM emulat de core-ul earlephilhower, stocat în ultimul
//   sector de flash (4 KB de la capătul memoriei), ÎN AFARA imaginii de
//   firmware. Un update prin fișier .uf2 rescrie doar paginile ocupate de
//   firmware, deci acest sector supraviețuiește update-urilor.

#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>

namespace Settings
{
    // Citește setările din flash; la primul boot sau date corupte
    // încarcă default-urile din Config.h și le salvează.
    void init();

    // Etajul unde e montat panoul de palier (Etj), 0 .. Config::Hardware::FLOORS-1
    uint8_t panelFloor();
    bool setPanelFloor(uint8_t floor);            // validează + persistă în flash

    // Timeout standby ecran (ms), în intervalul din Config::Timing
    uint32_t standbyTimeoutMs();
    bool setStandbyTimeoutMs(uint32_t timeoutMs); // validează + persistă în flash

    // Scrie explicit în flash starea curentă (setterii o fac deja)
    bool save();

    // Revine la default-urile din Config.h și persistă
    bool resetDefaults();
}

#endif // SETTINGS_H
