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
        constexpr uint8_t FLOORS = 18;       // Total etaje instalație


        // Default număr ascensor — folosit doar la primul boot sau dacă
        // zona persistentă e coruptă; valoarea reală stă în Settings (flash)
        constexpr uint16_t LIFT_NUMBER_DEFAULT = 1;
    }

    // =========================================================
    // 2. TIMING & TIMEOUT-URI (ms)
    // =========================================================
    namespace Timing
    {
        constexpr uint32_t BUTTON_INHIBIT_MS = 5000;     // Inhibare buton după apel; momentan nefolosită
        constexpr uint32_t BUTTON_STUCK_MS = 10000;      // Detecție buton blocat; momentan nefolosită
        constexpr uint32_t LED_BLINK_SLOW = 500;         // Perioadă clipire lentă LED; momentan nefolosită
        constexpr uint32_t LED_BLINK_FAST = 150;         // Perioadă clipire rapidă LED; momentan nefolosită
        constexpr uint32_t BOOT_TIME_MS = 5000;          // Timp boot; momentan nefolosită
    }

    // =========================================================
    // 3. DISPLAY / LCD — OFFSET-URI ȘI SETĂRI
    // =========================================================
    namespace Display
    {
        constexpr uint32_t SPI_CLOCK = 8000000UL;   // Viteză SPI (8 MHz)

        constexpr uint8_t ROTATION = 2;    // 0=portrait, 1=landscape, etc.
    }

    // =========================================================
    // 4. PROTOCOL ENCODER
    // =========================================================
    namespace Protocol
    {
        constexpr char START_MARKER = '<';
        constexpr char END_MARKER = '>';

        // Timeout fără pachet valid → lift considerat deconectat (Missing)
        // Ajustează în funcție de frecvența transmisiei controller-ului lift
        constexpr uint32_t PACKET_TIMEOUT_MS = 500;   // ← fostele 500ms hardcodate

    }

    namespace Ethernet
    {
        constexpr uint8_t MAC[6] = {0x3C, 0xAB, 0x72, 0x52, 0x68, 0x12};
        constexpr uint16_t ENCODER_PORT = 2000;
        constexpr uint16_t DEVICE_PORT = 2004;
        constexpr uint16_t SERVICE_PORT = 2005;
        constexpr uint32_t DHCP_TIMEOUT_MS = 1500;
        constexpr uint32_t DHCP_RESPONSE_TIMEOUT_MS = 250;
    }

    // =========================================================
    // 5. AUDIO
    // =========================================================
    namespace Audio
    {
        constexpr uint16_t BUZZER_FREQ = 4000;                   // momentan nefolosită
        constexpr uint32_t BOOT_BEEP_MS = 80;                    // momentan nefolosită

        // Gain audio — DEFAULT_GAIN e folosit doar la primul boot;
        // valoarea reală stă în Settings (flash protejat) și se aplică live
        constexpr float DEFAULT_GAIN = 0.5f;
        constexpr float MIN_GAIN = 0.0f;
        constexpr float MAX_GAIN = 4.0f;

        constexpr uint32_t ARRIVAL_FIRST_PULSE_MS = 60;          // momentan nefolosită
        constexpr uint32_t ARRIVAL_GAP_MS = 160;                 // momentan nefolosită
        constexpr uint32_t ARRIVAL_SECOND_PULSE_MS = 310;         // momentan nefolosită

        constexpr uint32_t ALERT_FIRST_PULSE_MS = 40;             // momentan nefolosită
        constexpr uint32_t ALERT_FIRST_GAP_MS = 80;               // momentan nefolosită
        constexpr uint32_t ALERT_SECOND_PULSE_MS = 120;           // momentan nefolosită
        constexpr uint32_t ALERT_SECOND_GAP_MS = 160;             // momentan nefolosită
        constexpr uint32_t ALERT_THIRD_PULSE_MS = 200;            // momentan nefolosită
    }
}

#endif // CONFIG_H
