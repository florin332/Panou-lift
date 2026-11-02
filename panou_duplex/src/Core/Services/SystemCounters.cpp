#include "SystemCounters.h"

#include <EEPROM.h>
#include <cstddef>
#include <cstdint>

namespace
{
    constexpr int BOOT_COUNTER_OFFSET = 0;
    constexpr int RESET_COUNTERS_OFFSET = 24; // Immediately follows the 20-byte Settings record at offset 4.
    constexpr uint32_t RESET_COUNTERS_MAGIC = 0x52535443UL; // "RSTC"
    constexpr uint16_t RESET_COUNTERS_VERSION = 1;

    struct PersistedResetCounters
    {
        uint32_t magic;
        uint16_t version;
        uint16_t reserved;
        uint32_t counts[4];
        uint32_t crc;
    };

    static_assert(sizeof(PersistedResetCounters) == 28, "Reset counter EEPROM layout changed");

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

    uint32_t computeCrc(const PersistedResetCounters &data)
    {
        return crc32(reinterpret_cast<const uint8_t *>(&data),
                     offsetof(PersistedResetCounters, crc));
    }

    bool isValid(const PersistedResetCounters &data)
    {
        if (data.magic != RESET_COUNTERS_MAGIC ||
            data.version != RESET_COUNTERS_VERSION ||
            data.reserved != 0 ||
            computeCrc(data) != data.crc)
            return false;

        for (uint8_t i = 0; i < 4; i++)
            if (data.counts[i] == UINT32_MAX) return false;

        return true;
    }

    PersistedResetCounters load()
    {
        PersistedResetCounters data{};
        EEPROM.get(RESET_COUNTERS_OFFSET, data);
        if (!isValid(data))
        {
            for (uint8_t i = 0; i < 4; i++) data.counts[i] = 0;
            data.magic = RESET_COUNTERS_MAGIC;
            data.version = RESET_COUNTERS_VERSION;
            data.reserved = 0;
        }
        return data;
    }

    void seal(PersistedResetCounters &data)
    {
        data.magic = RESET_COUNTERS_MAGIC;
        data.version = RESET_COUNTERS_VERSION;
        data.reserved = 0;
        data.crc = computeCrc(data);
    }
}

namespace SystemCounters
{
    uint16_t recordBoot(uint8_t resetReason)
    {
        uint16_t bootCount = 0;
        EEPROM.get(BOOT_COUNTER_OFFSET, bootCount);
        if (bootCount == 0xFFFF) bootCount = 0;
        bootCount++;

        PersistedResetCounters counters = load();
        if (resetReason >= 1 && resetReason <= 4 && counters.counts[resetReason - 1] != UINT32_MAX)
            counters.counts[resetReason - 1]++;
        seal(counters);

        EEPROM.put(BOOT_COUNTER_OFFSET, bootCount);
        EEPROM.put(RESET_COUNTERS_OFFSET, counters);
        EEPROM.commit();
        return bootCount;
    }

    void readResetCounts(uint32_t counts[4])
    {
        const PersistedResetCounters counters = load();
        for (uint8_t i = 0; i < 4; i++) counts[i] = counters.counts[i];
    }

    bool resetCategory(uint8_t category)
    {
        if (category < 1 || category > 4) return false;

        PersistedResetCounters counters = load();
        counters.counts[category - 1] = 0;
        seal(counters);
        EEPROM.put(RESET_COUNTERS_OFFSET, counters);
        return EEPROM.commit();
    }
}