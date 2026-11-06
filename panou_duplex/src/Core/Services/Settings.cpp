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
    constexpr uint16_t SETTINGS_VERSION = 4;
    constexpr uint16_t LEGACY_SETTINGS_VERSION_V3 = 3;
    constexpr uint16_t LEGACY_SETTINGS_VERSION_V2 = 2;
    constexpr size_t   EEPROM_SIZE      = 256;   // rezervat în sectorul de 4 KB
    constexpr int      SETTINGS_OFFSET  = 4;     // 0..3 = bootCounter (Application.cpp)
    constexpr uint8_t  UNSET_STATION_VALUE = 0xFF;

    // REGULĂ CRC: structura se scrie exclusiv câmp-cu-câmp (fără assignment pe
    // tot obiectul) și pornește din 'data{}' zero-inițializat static — padding-ul
    // inclus în CRC rămâne astfel mereu zero, atât în RAM cât și în flash.
    struct PersistedData
    {
        uint32_t magic;
        uint16_t version;
        uint8_t  panelFloor;
        uint8_t  secondCall;        // 0 = al doilea apel blocat (default), 1 = permis
        uint8_t  cabOrderSwapped;   // 0 = direct: cabina1→LCD1, cabina2→LCD2 (default); 1 = invers
        uint32_t standbyTimeoutMs;
        uint8_t  totalStations;
        uint8_t  groundStation;
        uint8_t  stationConfigWasSet;
        uint32_t crc32;
    };

    struct LegacyPersistedDataV3
    {
        uint32_t magic;
        uint16_t version;
        uint8_t  panelFloor;
        uint8_t  secondCall;
        uint8_t  cabOrderSwapped;
        uint32_t standbyTimeoutMs;
        uint8_t  floorMin;
        uint8_t  floorMax;
        uint8_t  floorRangeWasSet;
        uint32_t crc32;
    };

    struct LegacyPersistedDataV2
    {
        uint32_t magic;
        uint16_t version;
        uint8_t  panelFloor;
        uint8_t  secondCall;
        uint8_t  cabOrderSwapped;
        uint32_t standbyTimeoutMs;
        uint32_t crc32;
    };

    // Orice modificare de layout trebuie însoțită de bump SETTINGS_VERSION
    // și de actualizarea acestei dimensiuni.
    static_assert(sizeof(LegacyPersistedDataV2) == 20, "Legacy v2 settings layout changed");
    static_assert(sizeof(LegacyPersistedDataV3) == 24, "Legacy v3 settings layout changed");
    static_assert(sizeof(PersistedData) == 24, "Layout flash modificat: bump SETTINGS_VERSION + ajusteaza assert-ul");

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

    bool stationConfigIsValid(uint8_t totalStations, uint8_t groundStation)
    {
        return totalStations >= 1 && totalStations <= 30 && groundStation < totalStations;
    }

    void loadDefaults()
    {
        data.magic            = SETTINGS_MAGIC;
        data.version          = SETTINGS_VERSION;
        data.panelFloor       = Config::Hardware::PANEL_FLOOR_DEFAULT;
        data.secondCall       = Config::Hardware::SECOND_CALL_DEFAULT ? 1 : 0;
        data.cabOrderSwapped  = Config::Hardware::CAB_ORDER_DEFAULT ? 1 : 0;
        data.standbyTimeoutMs = Config::Timing::STANDBY_TIMEOUT_DEFAULT_MS;
        data.totalStations       = UNSET_STATION_VALUE;
        data.groundStation       = UNSET_STATION_VALUE;
        data.stationConfigWasSet = 0;
    }

    bool dataIsValid(const PersistedData &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == SETTINGS_VERSION &&
               floorIsValid(d.panelFloor) &&
               d.secondCall <= 1 &&
               d.cabOrderSwapped <= 1 &&
               standbyIsValid(d.standbyTimeoutMs) &&
               d.stationConfigWasSet <= 1 &&
               (d.stationConfigWasSet == 0 || stationConfigIsValid(d.totalStations, d.groundStation)) &&
               computeCrc(d) == d.crc32;
    }

    bool legacyDataIsValid(const LegacyPersistedDataV3 &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == LEGACY_SETTINGS_VERSION_V3 &&
               floorIsValid(d.panelFloor) &&
               d.secondCall <= 1 &&
               d.cabOrderSwapped <= 1 &&
               standbyIsValid(d.standbyTimeoutMs) &&
               crc32(reinterpret_cast<const uint8_t *>(&d),
                     offsetof(LegacyPersistedDataV3, crc32)) == d.crc32;
    }

    bool legacyDataIsValid(const LegacyPersistedDataV2 &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == LEGACY_SETTINGS_VERSION_V2 &&
               floorIsValid(d.panelFloor) &&
               d.secondCall <= 1 &&
               d.cabOrderSwapped <= 1 &&
               standbyIsValid(d.standbyTimeoutMs) &&
               crc32(reinterpret_cast<const uint8_t *>(&d),
                     offsetof(LegacyPersistedDataV2, crc32)) == d.crc32;
    }

    template <typename LegacyData>
    void migrateCommonSettings(const LegacyData &legacy)
    {
        loadDefaults();
        data.panelFloor = legacy.panelFloor;
        data.secondCall = legacy.secondCall;
        data.cabOrderSwapped = legacy.cabOrderSwapped;
        data.standbyTimeoutMs = legacy.standbyTimeoutMs;
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
            LegacyPersistedDataV3 legacyV3{};
            EEPROM.get(SETTINGS_OFFSET, legacyV3);
            if (legacyDataIsValid(legacyV3))
                migrateCommonSettings(legacyV3);
            else
            {
                LegacyPersistedDataV2 legacyV2{};
                EEPROM.get(SETTINGS_OFFSET, legacyV2);
                if (legacyDataIsValid(legacyV2))
                    migrateCommonSettings(legacyV2);
                else
                    loadDefaults();
            }
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

    bool secondCall()
    {
        return data.secondCall != 0;
    }

    bool setSecondCall(bool enabled)
    {
        data.secondCall = enabled ? 1 : 0;
        return save();
    }

    bool cabOrderSwapped()
    {
        return data.cabOrderSwapped != 0;
    }

    bool setCabOrderSwapped(bool swapped)
    {
        data.cabOrderSwapped = swapped ? 1 : 0;
        return save();
    }

    uint8_t totalStations()
    {
        return data.totalStations;
    }

    uint8_t groundStation()
    {
        return data.groundStation;
    }

    bool stationConfigWasSet()
    {
        return data.stationConfigWasSet != 0;
    }

    bool setStationConfig(uint8_t totalStations, uint8_t groundStation)
    {
        if (stationConfigWasSet() || !stationConfigIsValid(totalStations, groundStation))
            return false;

        const uint8_t previousTotalStations = data.totalStations;
        const uint8_t previousGroundStation = data.groundStation;
        data.totalStations = totalStations;
        data.groundStation = groundStation;
        data.stationConfigWasSet = 1;
        if (save())
            return true;

        data.totalStations = previousTotalStations;
        data.groundStation = previousGroundStation;
        data.stationConfigWasSet = 0;
        return false;
    }

    bool resetDefaults()
    {
        loadDefaults();
        return save();
    }
}
