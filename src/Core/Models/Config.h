//Hardware/Config.h (Parametrii Instalației specifice clădirii)

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

namespace Config
{
    namespace Hardware
    {
        constexpr uint8_t PANEL_FLOOR = 0;  // Parter
        constexpr uint8_t FLOORS      = 18; // Total etaje instalație
        constexpr uint8_t LIFTS       = 2;  // Mod Duplex
    }

    namespace Timing
    {
        constexpr uint32_t SCREEN_TIMEOUT_MS = 120000; 
        constexpr uint32_t SHIFT_PERIOD_MS   = 30000;  
        constexpr uint32_t BUTTON_INHIBIT_MS = 5000;   
        constexpr uint32_t BUTTON_STUCK_MS   = 10000;  
        constexpr uint32_t LED_BLINK_SLOW    = 500;     
        constexpr uint32_t LED_BLINK_FAST    = 150;     
        constexpr uint32_t BOOT_TIME_MS      = 5000;    
    }

    namespace Audio
    {
        constexpr uint16_t BUZZER_FREQ  = 4000; 
        constexpr uint32_t BOOT_BEEP_MS = 80;   

        constexpr uint32_t ARRIVAL_FIRST_PULSE_MS  = 60;  
        constexpr uint32_t ARRIVAL_GAP_MS          = 160; 
        constexpr uint32_t ARRIVAL_SECOND_PULSE_MS = 310; 

        constexpr uint32_t ALERT_FIRST_PULSE_MS   = 40;
        constexpr uint32_t ALERT_FIRST_GAP_MS     = 80;
        constexpr uint32_t ALERT_SECOND_PULSE_MS  = 120;
        constexpr uint32_t ALERT_SECOND_GAP_MS    = 160;
        constexpr uint32_t ALERT_THIRD_PULSE_MS   = 200;
    }

    namespace Display
    {
        constexpr uint32_t SPI_CLOCK = 8000000UL; 
    }

    namespace Protocol
    {
        constexpr uint32_t SERIAL_BAUD = 9600; 
        constexpr char START_MARKER    = '<';
        constexpr char END_MARKER      = '>';

        // Timpi fizici de stabilizare electrică pentru transceiverul MAX485 (Observația 4)
        constexpr uint32_t RS485_TX_SETTLE_US  = 50;  // Timp fixat înainte de trimiterea primului octet
        constexpr uint32_t RS485_TX_RELEASE_US = 50;  // Timp de descărcare după flush înainte de ascultare
        constexpr uint32_t RS485_LISTEN_SETTLE_US = 10;
    }
}

#endif // CONFIG_H
