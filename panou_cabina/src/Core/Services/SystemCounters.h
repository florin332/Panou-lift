#ifndef SYSTEM_COUNTERS_H
#define SYSTEM_COUNTERS_H

#include <cstdint>

namespace SystemCounters
{
    bool recordBoot(uint8_t resetReason);
    void readResetCounts(uint32_t counts[4]);
    bool resetCategory(uint8_t category);
}

#endif // SYSTEM_COUNTERS_H