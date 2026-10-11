// src/Core/Services/Settings.cpp
// Persistență în zona de flash protejată (EEPROM emulat, ultimul sector).
//
// Notă: adresele 0..3 sunt rezervate contorului de boot din Application.cpp;
// structura de setări începe la offsetul SETTINGS_OFFSET.

#include "Settings.h"
#include "Config.h"

#include <EEPROM.h>
#include <cstddef>

namespace
{
    constexpr uint32_t SETTINGS_MAGIC   = 0x44504C58UL;  // "DPLX"
    constexpr uint16_t SETTINGS_VERSION = 1;
    constexpr size_t   EEPROM_SIZE      = 256;   // rezervat în sectorul de 4 KB
    constexpr int      SETTINGS_OFFSET  = 4;     // 0..3 = bootCounter (Application.cpp)

    struct PersistedData
    {
        uint32_t magic;
        uint16_t version;
        uint8_t  panelFloor;
        uint32_t standbyTimeoutMs;
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

    bool floorIsValid(uint8_t floor)
    {
        return floor < Config::Hardware::FLOORS;
    }

    bool standbyIsValid(uint32_t timeoutMs)
    {
        return timeoutMs >= Config::Timing::STANDBY_TIMEOUT_MIN_MS &&
               timeoutMs <= Config::Timing::STANDBY_TIMEOUT_MAX_MS;
    }

    void loadDefaults()
    {
        data.magic            = SETTINGS_MAGIC;
        data.version          = SETTINGS_VERSION;
        data.panelFloor       = Config::Hardware::PANEL_FLOOR_DEFAULT;
        data.standbyTimeoutMs = Config::Timing::STANDBY_TIMEOUT_DEFAULT_MS;
    }

    bool dataIsValid(const PersistedData &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == SETTINGS_VERSION &&
               floorIsValid(d.panelFloor) &&
               standbyIsValid(d.standbyTimeoutMs) &&
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
        EEPROM.get(SETTINGS_OFFSET, data);

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
        EEPROM.put(SETTINGS_OFFSET, data);
        return EEPROM.commit();
    }

    uint8_t panelFloor()
    {
        return data.panelFloor;
    }

    bool setPanelFloor(uint8_t floor)
    {
        if (!floorIsValid(floor))
            return false;

        data.panelFloor = floor;
        return save();
    }

    uint32_t standbyTimeoutMs()
    {
        return data.standbyTimeoutMs;
    }

    bool setStandbyTimeoutMs(uint32_t timeoutMs)
    {
        if (!standbyIsValid(timeoutMs))
            return false;

        data.standbyTimeoutMs = timeoutMs;
        return save();
    }

    bool resetDefaults()
    {
        loadDefaults();
        return save();
    }
}
