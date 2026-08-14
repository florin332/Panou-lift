// src/Core/Models/Config.h
// Parametrii instalației și ai hardware-ului — MODIFICĂ DOAR AICI

#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

namespace Config
{
    // =========================================================
    // 1. HARDWARE INSTALAȚIE LIFT
    // =========================================================
    namespace Hardware
    {
        constexpr uint8_t PANEL_FLOOR = 0;   // Parter (etajul unde e panoul)
        constexpr uint8_t FLOORS = 18;       // Total etaje instalație
        constexpr uint8_t LIFTS = 2;         // Mod Duplex
    }

    // =========================================================
    // 2. TIMING & TIMEOUT-URI (ms)
    // =========================================================
    namespace Timing
    {
        constexpr uint32_t SCREEN_TIMEOUT_MS = 15000;   // Timeout standby ecran
        constexpr uint32_t SHIFT_PERIOD_MS = 30000;      // Perioadă schimb standby
        constexpr uint32_t BUTTON_INHIBIT_MS = 5000;     // Inhibare buton după apel
        constexpr uint32_t BUTTON_STUCK_MS = 10000;      // Detecție buton blocat
        constexpr uint32_t LED_BLINK_SLOW = 500;         // Perioadă clipire lentă LED
        constexpr uint32_t LED_BLINK_FAST = 150;         // Perioadă clipire rapidă LED
        constexpr uint32_t BOOT_TIME_MS = 5000;          // Timp boot
    }

    // =========================================================
    // 3. DISPLAY / LCD — OFFSET-URI ȘI SETĂRI
    // =========================================================
    namespace Display
    {
        constexpr uint32_t SPI_CLOCK = 8000000UL;   // Viteză SPI (8 MHz)

        // Offset-uri pentru ST7735 (depind de variantă chip + PCB)
        // Valori comune: BlackTab=(0,0), GreenTab=(1,2), ST7735S=(32,0)
        constexpr int16_t ROW_START = 1;   // ← Ajustează pentru LCD-ul tău
        constexpr int16_t COL_START = 2;   // ← Ajustează pentru LCD-ul tău

        constexpr uint8_t ROTATION = 0;    // 0=portrait, 1=landscape, etc.
    }

    // =========================================================
    // 4. PROTOCOL SERIAL / RS485
    // =========================================================
    namespace Protocol
    {
        constexpr uint32_t SERIAL_BAUD = 9600;
        constexpr char START_MARKER = '<';
        constexpr char END_MARKER = '>';

        // Timeout fără pachet valid → lift considerat deconectat (Missing)
        // Ajustează în funcție de frecvența transmisiei controller-ului lift
        constexpr uint32_t PACKET_TIMEOUT_MS = 500;   // ← fostele 500ms hardcodate

        // Timpi fizici de stabilizare electrică pentru transceiverul MAX485
        constexpr uint32_t RS485_TX_SETTLE_US = 50;
        constexpr uint32_t RS485_TX_RELEASE_US = 50;
        constexpr uint32_t RS485_LISTEN_SETTLE_US = 10;
    }

    // =========================================================
    // 5. AUDIO
    // =========================================================
    namespace Audio
    {
        constexpr uint16_t BUZZER_FREQ = 4000;
        constexpr uint32_t BOOT_BEEP_MS = 80;

        constexpr uint32_t ARRIVAL_FIRST_PULSE_MS = 60;
        constexpr uint32_t ARRIVAL_GAP_MS = 160;
        constexpr uint32_t ARRIVAL_SECOND_PULSE_MS = 310;

        constexpr uint32_t ALERT_FIRST_PULSE_MS = 40;
        constexpr uint32_t ALERT_FIRST_GAP_MS = 80;
        constexpr uint32_t ALERT_SECOND_PULSE_MS = 120;
        constexpr uint32_t ALERT_SECOND_GAP_MS = 160;
        constexpr uint32_t ALERT_THIRD_PULSE_MS = 200;
    }
}

#endif // CONFIG_H
