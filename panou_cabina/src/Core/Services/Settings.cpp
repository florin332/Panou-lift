// src/Core/Services/Settings.cpp
// Persistență în zona de flash protejată (EEPROM emulat, ultimul sector).

#include "Settings.h"
#include "Config.h"

#include <EEPROM.h>
#include <cstddef>
#include <cmath>

namespace
{
    constexpr uint32_t SETTINGS_MAGIC   = 0x4C494654UL;  // "LIFT"
    constexpr uint16_t SETTINGS_VERSION = 1;
    constexpr size_t   EEPROM_SIZE      = 256;           // rezervat în sectorul de 4 KB

    struct PersistedData
    {
        uint32_t magic;
        uint16_t version;
        uint16_t liftNumber;
        float    audioGain;
        uint32_t crc32;
    };

    PersistedData data{};
    bool loaded = false;

    uint32_t crc32(const uint8_t *bytes, size_t length)
    {
        uint32_t crc = 0xFFFFFFFFUL;
        for (size_t i = 0; i < length; i++)
        {
            crc ^= bytes[i];
            for (uint8_t bit = 0; bit < 8; bit++)
                crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
        }
        return ~crc;
    }

    uint32_t computeCrc(const PersistedData &d)
    {
        return crc32(reinterpret_cast<const uint8_t *>(&d),
                     offsetof(PersistedData, crc32));
    }

    bool gainIsValid(float gain)
    {
        return !std::isnan(gain) &&
               gain >= Config::Audio::MIN_GAIN &&
               gain <= Config::Audio::MAX_GAIN;
    }

    float clampGain(float gain)
    {
        if (std::isnan(gain) || gain < Config::Audio::MIN_GAIN)
            return Config::Audio::MIN_GAIN;
        if (gain > Config::Audio::MAX_GAIN)
            return Config::Audio::MAX_GAIN;
        return gain;
    }

    void loadDefaults()
    {
        data.magic      = SETTINGS_MAGIC;
        data.version    = SETTINGS_VERSION;
        data.liftNumber = Config::Hardware::LIFT_NUMBER_DEFAULT;
        data.audioGain  = Config::Audio::DEFAULT_GAIN;
    }

    bool dataIsValid(const PersistedData &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == SETTINGS_VERSION &&
               d.liftNumber >= 1 &&
               gainIsValid(d.audioGain) &&
               computeCrc(d) == d.crc32;
    }
}

namespace Settings
{
    void init()
    {
        if (loaded)
            return;

        EEPROM.begin(EEPROM_SIZE);
        EEPROM.get(0, data);

        if (!dataIsValid(data))
        {
            loadDefaults();
            save();
        }

        loaded = true;
    }

    bool save()
    {
        data.magic   = SETTINGS_MAGIC;
        data.version = SETTINGS_VERSION;
        data.crc32   = computeCrc(data);
        EEPROM.put(0, data);
        return EEPROM.commit();
    }

    uint16_t liftNumber()
    {
        return data.liftNumber;
    }

    bool setLiftNumber(uint16_t number)
    {
        if (number < 1)
            return false;

        data.liftNumber = number;
        return save();
    }

    float audioGain()
    {
        return data.audioGain;
    }

    bool setAudioGain(float gain)
    {
        if (!gainIsValid(gain))
            return false;

        data.audioGain = clampGain(gain);
        return save();
    }

    bool resetDefaults()
    {
        loadDefaults();
        return save();
    }
}
