#include "ConfigurationService.h"
#include "Settings.h"
#include "Config.h"
#include "SystemCounters.h"
#include <EEPROM.h>
#include <hardware/regs/watchdog.h>
#include "hardware/structs/vreg_and_chip_reset.h"
#include <hardware/structs/watchdog.h>

namespace
{
    constexpr int BOOT_COUNT_ADDR = 256;
    constexpr int EEPROM_SIZE = 258;

    uint8_t hardwareResetReason() {
        const uint32_t watchdogReason = watchdog_hw->reason;
        const uint32_t chipResetReason = vreg_and_chip_reset_hw->chip_reset;
        vreg_and_chip_reset_hw->chip_reset =
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS |
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_BITS |
            VREG_AND_CHIP_RESET_CHIP_RESET_HAD_PSM_RESTART_BITS;

        if (watchdog_hw->scratch[0] == Config::Hardware::RESET_MAGIC_BUTTON) {
            watchdog_hw->scratch[0] = 0;
            return 4;
        }
        if (watchdogReason & (WATCHDOG_REASON_TIMER_BITS | WATCHDOG_REASON_FORCE_BITS)) return 2;
        // Categoria 1: POR/brownout sau reset hardware pe pinul RUN.
        // Pe Marble Pico, reteaua RC a butonului RST tine RUN activ scurt
        // dupa POR, deci la intreruperea alimentarii ultimul reset vazut
        // este RUN (HAD_POR ramane nesetat). Verificarea watchdog de mai sus
        // are prioritate, deoarece resetul watchdog seteaza si el HAD_RUN.
        if (chipResetReason & (VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS |
                               VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_BITS)) return 1;
        // Categoria 3: restart din debug (PSM) sau cauza nedeterminata.
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
        const uint8_t resetReason = hardwareResetReason();
        state.system.lastResetReason = resetReason;
        SystemCounters::recordBoot(resetReason);
    }

    uint16_t liftNumber() { return Settings::liftNumber(); }
    bool setLiftNumber(uint16_t number) { return Settings::setLiftNumber(number); }
    float audioGain() { return Settings::audioGain(); }
    bool resetDefaults() { return Settings::resetDefaults(); }
}