#include "ConfigurationService.h"
#include "Settings.h"
#include <EEPROM.h>
#include "hardware/structs/vreg_and_chip_reset.h"

namespace
{
    constexpr int BOOT_COUNT_ADDR = 256;
    constexpr int EEPROM_SIZE = 258;

    uint8_t hardwareResetReason() {
        const uint32_t reason = vreg_and_chip_reset_hw->chip_reset;
        if (reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_BITS) return 2;
        if (reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS) return 1;
        return 3;
    }
}

namespace ConfigurationService
{
    void init() {
        Settings::init();
    }

    void initializeStartupState(SharedPanel &state) {
        EEPROM.begin(EEPROM_SIZE);
        uint16_t bootCount = 0;
        EEPROM.get(BOOT_COUNT_ADDR, bootCount);
        if (bootCount == 0xFFFF) bootCount = 0;
        ++bootCount;
        EEPROM.put(BOOT_COUNT_ADDR, bootCount);
        EEPROM.commit();

        state.system.bootCounter = bootCount;
        state.system.lastResetReason = hardwareResetReason();
    }

    uint16_t liftNumber() { return Settings::liftNumber(); }
    bool setLiftNumber(uint16_t number) { return Settings::setLiftNumber(number); }
    float audioGain() { return Settings::audioGain(); }
    bool resetDefaults() { return Settings::resetDefaults(); }
}