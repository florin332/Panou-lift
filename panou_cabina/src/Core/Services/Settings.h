// src/Core/Services/Settings.h
// Setări persistente per-panou (număr ascensor, gain audio).
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

    // Număr ascensor deservit de acest panou (1..65535)
    uint16_t liftNumber();
    bool setLiftNumber(uint16_t number);   // validează + persistă în flash

    // Gain audio (0.0 .. Config::Audio::MAX_GAIN)
    float audioGain();
    bool setAudioGain(float gain);         // clamp + persistă în flash

    // Scrie explicit în flash starea curentă (setterii o fac deja)
    bool save();

    // Revine la default-urile din Config.h și persistă
    bool resetDefaults();
}

#endif // SETTINGS_H
